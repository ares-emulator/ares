// Experimental native host. stdout is exclusively the AEF2 binary protocol.
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>
#include <csignal>
#ifndef _WIN32
#include <unistd.h>
#endif
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif
#include <desktop-ui/desktop-ui.hpp>
namespace ruby { Video video; Audio audio; Input input; }

// Resource/config lookup shared with existing loaders, without changing user settings.
auto locate(const string& name) -> string {
  for(auto base : {Path::program(), Path::resources(), string{Path::userData(), "ares/"},
                   string{ARES_SOURCE_ROOT, "mia/"}, string{ARES_SOURCE_ROOT, "desktop-ui/"}}) {
    string candidate{base, name};
    if(inode::exists(candidate)) return candidate;
  }
  return {Path::userData(), "ares/", name};
}

using Clock = std::chrono::steady_clock;
struct Host : ares::Platform {
  ares::Node::System root;
  std::shared_ptr<HID::Joypad> joypad;
  std::shared_ptr<HID::Keyboard> keyboard;
  std::shared_ptr<HID::Mouse> mouse;
  string systemName = "Game Boy";
  std::atomic<double> refreshRate{59.7275};
  ruby::Audio output;
  std::vector<ares::Node::Audio::Stream> streams;
  std::atomic<bool> quit{false}, paused{false}, reset{false};
  std::atomic<uint64_t> controls{0}; // sequence << 32 | button mask
  std::atomic<uint32_t> appliedInput{0};
  bool sound = true;
  FILE* frameOutput = stdout;
  std::mutex mutex;
  std::condition_variable wake;
  bool credit = false, available = false;
  std::vector<uint8_t> latest;
  uint32_t sequence = 0;
  Clock::time_point started = Clock::now(), captured;

  static void put32(uint8_t* p, uint32_t v) {
    for(int i = 0; i < 4; i++) p[i] = v >> (8 * i);
  }
  static void put64(uint8_t* p, double v) {
    uint64_t bits; std::memcpy(&bits, &v, 8);
    for(int i = 0; i < 8; i++) p[i] = bits >> (8 * i);
  }
  auto pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> override {
    return emulator ? emulator->pak(node) : nullptr;
  }
  void setupInput() {
    joypad = std::make_shared<HID::Joypad>(); joypad->setID(0xe1ec0003);
    for(auto name : {"X", "Y", "RX", "RY"}) joypad->axes().append(name);
    for(unsigned i = 0; i < 32; i++) joypad->buttons().append({i});
    keyboard = std::make_shared<HID::Keyboard>(); keyboard->setID(0xe1ec0001);
    mouse = std::make_shared<HID::Mouse>(); mouse->setID(0xe1ec0002);
    mouse->appendAxis("X"); mouse->appendAxis("Y");
    for(auto name : {"Left", "Middle", "Right", "Extra"}) mouse->buttons().append(name);
    inputManager.devices = {joypad, keyboard, mouse};
    auto bind = [](InputMapping& mapping, u64 id, unsigned group, unsigned index, const char* qualifier = "") {
      mapping.unbind();
      mapping.bind(0, string{"0x", hex(id), "/", group, "/", index, qualifier});
    };
    auto& pad = virtualPorts[0].pad;
    InputMapping* buttons[] = {&pad.up, &pad.down, &pad.left, &pad.right,
      &pad.south, &pad.east, &pad.west, &pad.north, &pad.select, &pad.start,
      &pad.l_bumper, &pad.r_bumper, &pad.l_trigger, &pad.r_trigger, &pad.lstick_click, &pad.rstick_click,
      &pad.one, &pad.two, &pad.three, &pad.four, &pad.five, &pad.six, &pad.seven, &pad.eight,
      &pad.nine, &pad.zero, &pad.star, &pad.clear, &pad.pound, &pad.point, &pad.end};
    for(unsigned i = 0; i < std::size(buttons); i++) bind(*buttons[i], joypad->id(), HID::Joypad::Button, i);
    InputMapping* halves[] = {&pad.lstick_left, &pad.lstick_right, &pad.lstick_up, &pad.lstick_down,
      &pad.rstick_left, &pad.rstick_right, &pad.rstick_up, &pad.rstick_down};
    for(unsigned i = 0; i < 8; i++) bind(*halves[i], joypad->id(), HID::Joypad::Axis, i / 2, i % 2 ? "/Hi" : "/Lo");
    auto& pointer = virtualPorts[0].mouse;
    bind(pointer.x, mouse->id(), HID::Mouse::Axis, 0); bind(pointer.y, mouse->id(), HID::Mouse::Axis, 1);
    InputMapping* mouseButtons[] = {&pointer.left, &pointer.middle, &pointer.right, &pointer.extra};
    for(unsigned i = 0; i < 4; i++) bind(*mouseButtons[i], mouse->id(), HID::Mouse::Button, i);
    // Imported physical-device assignments must not override the Electron virtual pad.
    for(auto& entry : emulators) for(auto& port : entry->ports) for(auto& device : port.devices) {
      for(auto& node : device.inputs) if(node.directMapping) node.directMapping->unbind();
      for(auto& pair : device.pairs) {
        if(pair.directMappingLo) pair.directMappingLo->unbind();
        if(pair.directMappingHi) pair.directMappingHi->unbind();
      }
    }
    program.externalInput = true;
    program.keyboardCaptured = true;
  }
  auto status(string_view message) -> void override { std::cerr << string{message}.data() << "\n"; }
  auto refreshRateHint(double rate) -> void override { if(rate > 1 && rate < 300) refreshRate = rate; }
  auto attach(ares::Node::Object node) -> void override {
    if(auto stream = node->cast<ares::Node::Audio::Stream>()) {
      stream->setResamplerFrequency(48000);
      streams.push_back(stream);
    }
  }
  auto audio(ares::Node::Audio::Stream) -> void override {
    if(streams.empty()) return;
    for(;;) {
      for(auto& stream : streams) if(!stream->pending()) return;
      double mixed[2]{};
      for(auto& stream : streams) {
        double samples[2]{};
        auto channels = stream->read(samples);
        mixed[0] += samples[0];
        mixed[1] += samples[channels == 1 ? 0 : 1];
      }
      for(auto& sample : mixed) sample = std::clamp(sample, -1.0, 1.0) * 0.4;
      if(sound) output.output(mixed);
    }
  }
  auto input(ares::Node::Input::Input node) -> void override {
    if(emulator) emulator->input(node);
    appliedInput = controls.load() >> 32;
  }
  auto detach(ares::Node::Object node) -> void override {
    if(auto stream = node->cast<ares::Node::Audio::Stream>()) std::erase(streams, stream);
  }
  auto video(ares::Node::Video::Screen screen, const u32* data, u32 pitch, u32 width, u32 height) -> void override {
    auto now = Clock::now();
    std::lock_guard lock(mutex);
    latest.resize(48 + size_t(width) * height * 4);
    auto p = latest.data();
    std::memcpy(p, "AEF2", 4);
    put32(p + 4, width * height * 4); put32(p + 8, width); put32(p + 12, height);
    put32(p + 16, ++sequence); put32(p + 20, appliedInput);
    put64(p + 24, std::chrono::duration<double, std::milli>(now - started).count());
    // Explicit conversion handles ARGB words and non-contiguous source rows.
    double aspect = double(width) / height;
    if(screen && screen->height() && screen->scaleY() && screen->aspectY()) {
      aspect = (screen->width() * screen->scaleX() * screen->aspectX()) /
               (screen->height() * screen->scaleY() * screen->aspectY());
      if(screen->rotation() == 90 || screen->rotation() == 270) aspect = 1.0 / aspect;
    }
    put64(p + 40, aspect);
    p += 48;
    for(uint32_t y = 0; y < height; y++) {
      auto row = reinterpret_cast<const u32*>(reinterpret_cast<const uint8_t*>(data) + y * pitch);
      for(uint32_t x = 0; x < width; x++) {
        auto color = row[x];
        *p++ = color >> 16; *p++ = color >> 8; *p++ = color; *p++ = 255;
      }
    }
    captured = now; available = true; wake.notify_one();
  }
  void writeFrames() {
    std::vector<uint8_t> frame;
    while(!quit) {
      {
        std::unique_lock lock(mutex);
        wake.wait(lock, [&] { return quit || (credit && available); });
        if(quit) break;
        frame.swap(latest); available = false; credit = false;
        put64(frame.data() + 32, std::chrono::duration<double, std::milli>(Clock::now() - captured).count());
      }
      if(std::fwrite(frame.data(), 1, frame.size(), frameOutput) != frame.size() || std::fflush(frameOutput)) {
        quit = true; break;
      }
    }
  }
  void readCommands() {
    std::string line;
    while(std::getline(std::cin, line)) {
      std::istringstream command(line);
      std::string name; command >> name;
      if(name == "ack") { std::lock_guard lock(mutex); credit = true; wake.notify_one(); }
      if(name == "pause") { unsigned value; if(command >> value) paused = value != 0; }
      if(name == "reset") reset = true;
      if(name == "input") {
        uint32_t seq, mask;
        if(command >> seq >> mask) {
          int axes[4]{}; command >> axes[0] >> axes[1] >> axes[2] >> axes[3];
          std::lock_guard lock(program.inputMutex);
          if(joypad) {
            for(unsigned i = 0; i < 4; i++) joypad->axes().input(i).setValue(std::clamp(axes[i], -32767, 32767));
            for(unsigned i = 0; i < 32; i++) joypad->buttons().input(i).setValue((mask >> i) & 1);
          }
          controls = (uint64_t(seq) << 32) | mask;
        }
      }
      if(name == "key" && keyboard) {
        std::string key; int value;
        if(command >> key >> value && key.size() <= 32) {
          std::lock_guard lock(program.inputMutex);
          auto found = keyboard->buttons().find(key.c_str());
          if(!found && keyboard->buttons().size() < 128) { keyboard->buttons().append(key.c_str()); found = keyboard->buttons().find(key.c_str()); }
          if(found) keyboard->buttons().input(*found).setValue(value != 0);
        }
      }
      if(name == "mouse" && mouse) {
        int x, y; unsigned mask;
        if(command >> x >> y >> mask) {
          std::lock_guard lock(program.inputMutex);
          mouse->motion(0).move(std::clamp(x, -32767, 32767)); mouse->motion(1).move(std::clamp(y, -32767, 32767));
          for(unsigned i = 0; i < 4; i++) mouse->buttons().input(i).setValue((mask >> i) & 1);
        }
      }
      if(name == "clear") {
        std::lock_guard lock(program.inputMutex);
        if(keyboard) for(auto& key : keyboard->buttons()) key.setValue(0);
        if(mouse) for(auto& button : mouse->buttons()) button.setValue(0);
      }
      if(name == "quit") break;
    }
    quit = true; wake.notify_all();
  }
};

int main(int argc, char** argv) {
#ifdef _WIN32
  _setmode(_fileno(stdout), _O_BINARY);
#else
  std::signal(SIGPIPE, SIG_IGN);
#endif
  Host host;
  // Core/MIA diagnostics may use print()/printf()/cout. Keep their stdout
  // separate from the protocol, including diagnostics emitted during gameplay.
#ifdef _WIN32
  host.frameOutput = _fdopen(_dup(_fileno(stdout)), "wb");
  _dup2(_fileno(stderr), _fileno(stdout));
#else
  host.frameOutput = fdopen(dup(fileno(stdout)), "wb");
  dup2(fileno(stderr), fileno(stdout));
#endif
  if(!host.frameOutput) { std::cerr << "Cannot open frame transport\n"; return 1; }
  bool synthetic = false, identifyOnly = false, listSystems = false;
  unsigned width = 640, height = 480;
  std::string rom, saveDirectory, settingsFile, selectedSystem;
  std::vector<string> extraMedia;
  for(int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if(arg == "--silent") host.sound = false;
    else if(arg == "--synthetic") synthetic = true;
    else if(arg == "--1080p") { width = 1920; height = 1080; }
    else if(arg == "--systems") listSystems = true;
    else if(arg == "--settings" && i + 1 < argc) settingsFile = argv[++i];
    else if(arg == "--system" && i + 1 < argc) selectedSystem = argv[++i];
    else if(arg == "--media" && i + 1 < argc) extraMedia.push_back(argv[++i]);
    else if(arg == "--save-dir" && i + 1 < argc) saveDirectory = argv[++i];
    else if(arg == "--identify" && i + 1 < argc) { identifyOnly = true; rom = argv[++i]; }
    else if(arg == "--rom" && i + 1 < argc) rom = argv[++i];
    else { std::cerr << "Usage: ares-electron-host [--rom file | --identify file | --synthetic [--1080p]] [--silent] [--save-dir directory]\n"; return 2; }
  }
  inputManager.create();
  Emulator::construct();
  program.kiosk = true; program.noFilePrompt = true;
  if(listSystems) { for(auto& entry : emulators) std::fprintf(host.frameOutput, "%s\n", entry->name.data()); return 0; }
  if(!synthetic && rom.empty()) { std::cerr << "A ROM is required\n"; return 2; }
  if(settingsFile.empty()) settingsFile = locate("settings.bml").data();
  static_cast<Markup::Node&>(settings) = BML::unserialize(string::read(settingsFile.c_str()), " ");
  settings.process(true); // Read configuration only; never rewrite the desktop settings.
  if(saveDirectory.empty()) saveDirectory = std::string(Path::program().data()) + "electron-data/saves/";
  if(saveDirectory.back() != '/') saveDirectory += '/';
  directory::create(saveDirectory.c_str());
  settings.paths.saves = saveDirectory.c_str();
  mia::setSaveLocation([&]() -> string { return settings.paths.saves; });
  mia::setHomeLocation([&]() -> string {
    if(settings.paths.home) return settings.paths.home;
    return {saveDirectory.c_str(), "/../systems/"};
  });
  ares::platform = &host;
  ares::Memory::FixedAllocator::get();
  if(!synthetic) {
    auto matches = mia::identify(rom.c_str());
    // MIA considers monochrome GB cartridges compatible with both GB models.
    if(matches.size() == 2 && std::ranges::all_of(matches, [](const auto& name) {
      return name == "Game Boy" || name == "Game Boy Color";
    })) {
      auto probe = mia::Medium::create("Game Boy");
      if(probe->load(rom.c_str()) != successful) return 2;
      auto bytes = probe->pak->read("program.rom"); bytes->seek(0x143);
      matches = {bytes->read() & 0x80 ? "Game Boy Color" : "Game Boy"};
    }
    if(!selectedSystem.empty()) matches = {selectedSystem.c_str()};
    // The registry, rather than a second hand-maintained whitelist, determines support.
    std::erase_if(matches, [](const auto& name) {
      return std::ranges::none_of(emulators, [&](const auto& entry) { return entry->name == name; });
    });
    if(matches.empty()) { std::cerr << "Could not identify a compiled system. Select the system explicitly.\n"; return 2; }
    if(identifyOnly) { for(auto& match : matches) std::fprintf(host.frameOutput, "%s\n", match.data()); return 0; }
    if(matches.size() != 1) { std::cerr << "Ambiguous ROM: select a system explicitly.\n"; return 2; }
    for(auto& entry : emulators) if(entry->name == matches.front()) emulator = entry;
    if(!emulator) { std::cerr << "System is not compiled into this host\n"; return 2; }
    host.systemName = emulator->name;
    program.startGameLoad = extraMedia;
    settings.general.noFilePrompt = extraMedia.empty();
    settings.boot.fast = true;
    settings.nintendo64.quality = "SD";
    settings.nintendo64.supersampling = false;
    settings.developer.forceInterpreter = false;
    host.setupInput();
    if(host.sound) {
      if(!host.output.create()) { std::cerr << "Native audio unavailable; use --silent to test without it\n"; return 1; }
      host.output.setBlocking(true);
    }
    if(!emulator->load(string{rom.c_str()})) {
      if(emulator->root) emulator->root->unload();
      emulator.reset(); return 1;
    }
    host.root = emulator->root;
    program.paletteUpdate();
  }
  std::cerr << "ready: " << (synthetic ? "synthetic RGBA source" : host.systemName.data())
            << "; audio=" << (!synthetic && host.sound ? "native SDL" : "off") << "\n";
  std::thread reader([&] { host.readCommands(); });
  std::thread writer([&] { host.writeFrames(); });
  // Padded synthetic rows exercise pitch handling, not just tightly packed data.
  std::vector<u32> pixels(synthetic ? (width + 8) * height : 0);
  auto deadline = Clock::now();
  uint32_t tick = 0;
  while(!host.quit) {
    if(host.reset.exchange(false) && host.root) { host.output.clear(); host.root->power(); }
    if(host.paused) {
      if(host.sound && !synthetic) host.output.clear();
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      deadline = Clock::now(); continue;
    }
    if(synthetic) {
      host.appliedInput = host.controls.load() >> 32;
      for(unsigned y = 0; y < height; y++) for(unsigned x = 0; x < width; x++) {
        pixels[y * (width + 8) + x] = 0xff000000u | ((x + tick) % 256 << 16) | ((y + tick) % 256 << 8) | (tick % 256);
      }
      host.video({}, pixels.data(), (width + 8) * 4, width, height); ++tick;
    } else host.root->run();
    if(synthetic || !host.sound) {
      deadline += std::chrono::nanoseconds(synthetic ? 16666667 : int64_t(1e9 / host.refreshRate.load()));
      std::this_thread::sleep_until(deadline);
      if(Clock::now() - deadline > std::chrono::milliseconds(100)) deadline = Clock::now();
    }
  }
  host.quit = true; host.wake.notify_all();
  writer.join(); reader.join();
  if(host.root) {
    emulator->unload(); host.streams.clear(); host.root.reset(); emulator.reset();
  }
  std::cerr << "stopped\n";
  ares::platform = nullptr;
}
