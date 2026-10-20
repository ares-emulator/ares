// Experimental native host. stdout is exclusively the AEF2 binary protocol.
#include <atomic>
#include <array>
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
#include <n64/n64.hpp>
#include <ares/frame-observer.hpp>
#include <nall/gdb/server.hpp>
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

#include "native/session-service.hpp"
#include "native/n64-service.hpp"
#include "native/input-service.hpp"
#include "native/trace-service.hpp"
#include "native/dma-service.hpp"
#include "native/profiler-service.hpp"
#include "native/tap-service.hpp"
#include "native/frame-service.hpp"
#include "native/audio-service.hpp"
#include "native/recording-service.hpp"
#include "native/state-snapshot.hpp"
#include "native/state-loader.hpp"

using Clock = std::chrono::steady_clock;
struct Host : ares::Platform {
  InputService injectedInput;
  FrameService captures;
  AudioService audioCapture;
  RecordingService recording;
  StateSnapshot stateSnapshot;
  StateLoader stateLoader;
  std::atomic<bool> takeoverRequested{false};
  ares::Node::System root;
  std::shared_ptr<HID::Joypad> joypad;
  std::shared_ptr<HID::Keyboard> keyboard;
  std::shared_ptr<HID::Mouse> mouse;
  string systemName = "Game Boy";
  std::atomic<double> refreshRate{59.7275};
  ruby::Audio output;
  std::vector<ares::Node::Audio::Stream> streams;
  SessionService service;
  MemoryService memory;
  std::atomic<bool> quit{false};
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
      audioCapture.push(mixed[0], mixed[1]);
      if(sound && !service.execution.active) output.output(mixed);
    }
  }
  auto input(ares::Node::Input::Input node) -> void override {
    if(!injectedInput.apply(node, service.epoch, ares::Nintendo64::executionCoordinate.frame) && emulator) emulator->input(node);
    appliedInput = controls.load() >> 32;
  }
  auto detach(ares::Node::Object node) -> void override {
    captures.forget(node.get());
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
    captures.observe(screen.get(), latest.data() + 48, width, height, sequence, screen ? 0 : service.epoch, screen ? 0 : service.execution.frame);
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
    std::array<int, 4> previousAxes{};
    // Bound input before allocating an arbitrarily large line.
    char ch;
    while(std::cin.get(ch)) {
      if(ch != '\n') {
        if(line.size() >= 4096) break;
        line += ch; continue;
      }
      std::istringstream command(line);
      std::string name; command >> name;
      if(name != "ack" && name != "input" && name != "key" && name != "mouse" && name != "clear" && name != "quit") {
        if(!service.output || !service.enqueue(line)) break;
        line.clear(); continue;
      }
      if(name == "ack") { std::lock_guard lock(mutex); credit = true; wake.notify_one(); }
      if(name == "input") {
        uint32_t seq, mask;
        if(command >> seq >> mask) {
          int axes[4]{}; command >> axes[0] >> axes[1] >> axes[2] >> axes[3];
          bool moved = (mask & ~uint32_t(controls.load())) != 0;
          for(unsigned i = 0; i < 4; ++i) {
            axes[i] = std::clamp(axes[i], -32767, 32767);
            moved |= std::abs(axes[i]) > 4096 && std::abs(axes[i] - previousAxes[i]) > 4096;
            previousAxes[i] = axes[i];
          }
          if(moved) takeoverRequested = true;
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
          if(found) {
            auto& button = keyboard->buttons().input(*found);
            if(value && !button.value()) takeoverRequested = true;
            button.setValue(value != 0);
          }
        }
      }
      if(name == "mouse" && mouse) {
        int x, y; unsigned mask;
        if(command >> x >> y >> mask) {
          std::lock_guard lock(program.inputMutex);
          mouse->motion(0).move(std::clamp(x, -32767, 32767)); mouse->motion(1).move(std::clamp(y, -32767, 32767));
          for(unsigned i = 0; i < 4; i++) {
            auto& button = mouse->buttons().input(i);
            if(((mask >> i) & 1) && !button.value()) takeoverRequested = true;
            button.setValue((mask >> i) & 1);
          }
        }
      }
      if(name == "clear") {
        std::lock_guard lock(program.inputMutex);
        if(keyboard) for(auto& key : keyboard->buttons()) key.setValue(0);
        if(mouse) for(auto& button : mouse->buttons()) button.setValue(0);
      }
      if(name == "quit") break;
      line.clear();
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
    if(arg == "--debug-capture") host.captures.enabled = true;
    else if(arg == "--debug-provenance") ares::Nintendo64::dmaProvenance.enabled = true;
    else if(arg == "--silent") host.sound = false;
    else if(arg == "--control-session" && i + 1 < argc) {
      host.service.session = argv[++i];
      if(host.service.session.empty() || host.service.session.size() > 64 || host.service.session.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_") != std::string::npos) return 2;
#ifdef _WIN32
      host.service.output = _fdopen(3, "wb");
#else
      host.service.output = fdopen(3, "wb");
#endif
      if(!host.service.output) return 2;
    }
    else if(arg == "--paused") host.service.paused = true;
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
  ares::Nintendo64::dmaProvenance.reset(host.service.epoch);
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
  auto memoryRoot = host.root;
  std::array<uint8_t, 256> fixtureMemory;
  if(synthetic) {
    // Explicit synthetic debugger nodes exercise access metadata without a ROM.
    for(size_t i = 0; i < fixtureMemory.size(); ++i) fixtureMemory[i] = i;
    memoryRoot = std::make_shared<ares::Core::System>("Synthetic");
    auto ram = memoryRoot->append<ares::Node::Debugger::Memory>("Fixture RAM");
    ram->setSize(fixtureMemory.size());
    ram->setRead([&](u32 offset) -> u8 { return fixtureMemory[offset]; });
    ram->setWrite([&](u32 offset, u8 byte) { fixtureMemory[offset] = byte; });
    auto rom = memoryRoot->append<ares::Node::Debugger::Memory>("Fixture read-only");
    rom->setSize(fixtureMemory.size());
    rom->setRead([&](u32 offset) -> u8 { return fixtureMemory[offset]; });
    auto absent = memoryRoot->append<ares::Node::Debugger::Memory>("Fixture unavailable");
    absent->setSize(16);
  }
  host.memory.bind(memoryRoot, host.service.session, memoryRoot && memoryRoot->name() == "Nintendo 64");
  std::thread reader([&] { host.readCommands(); });
  std::thread writer([&] { host.writeFrames(); });
  std::thread replies;
  if(host.service.output) replies = std::thread([&] { host.service.writeReplies(); });
  // Padded synthetic rows exercise pitch handling, not just tightly packed data.
  std::vector<u32> pixels(synthetic ? (width + 8) * height : 0);
  auto deadline = Clock::now();
  uint32_t tick = 0;
  unsigned syntheticPhase = 0;
  bool n64 = !synthetic && host.root && host.root->name() == "Nintendo 64";
  auto& execution = host.service.execution;
  N64Service n64Service; n64Service.available = n64;
  TraceService traceService; traceService.available = n64;
  TapService tapService;
  ProfilerService profilerService; profilerService.available = n64;
  DmaService dmaService; dmaService.available = n64;
  execution.cpuSupported = n64;
  host.injectedInput.available = n64;
  ares::Video::frameSubmittedObserver = [&](const void* screen) {
    host.captures.submitted(screen, host.service.epoch, n64 ? ares::Nintendo64::executionCoordinate.frame : execution.frame);
  };
  host.captures.onCapture = [&](const uint8_t* pixels, uint32_t w, uint32_t h, uint64_t epoch, uint64_t frame) { host.recording.capture(pixels, w, h, epoch, frame); };
  host.service.videoRequest = [&](const std::string& method, const std::vector<std::string>& args, const std::string& owner) {
    if(method.starts_with("video.record.")) {
      auto result = host.recording.execute(method, args, owner, host.service.epoch, execution.frame, execution.frameKind);
      if(result.error.empty() && method == "video.record.start") host.captures.enabled = true;
      return result;
    }
    return host.captures.execute(method, args, owner, host.service.epoch, execution.frame, n64, execution.frameKind, host.service.paused);
  };
  host.service.audioRequest = [&](const std::string& method, const std::vector<std::string>& args, const std::string& owner) {
    return host.audioCapture.execute(method, args, owner, !host.streams.empty());
  };
  host.service.onReleaseOwner = [&](const std::string& owner) { host.injectedInput.releaseOwner(owner); profilerService.releaseOwner(owner); host.captures.releaseOwner(owner); host.audioCapture.releaseOwner(owner); host.recording.releaseOwner(owner); host.stateSnapshot.releaseOwner(owner); host.stateLoader.releaseOwner(owner); };
  host.service.beforeRequest = [&](const std::string& method, const std::string& owner) -> std::string {
    auto& load = host.stateLoader;
    if(load.upload && Clock::now() >= load.upload->deadline) load.reset();
    if(load.upload && (method == "session.pause" || method == "session.resume") && owner != "ui" && owner != load.upload->owner) return "busy";
    if(!load.upload || method.starts_with("n64.state.load")) return {};
    static const std::vector<std::string> allowed = {
      "session.status", "session.pause", "session.resume", "session.reset", "execution.status", "execution.cancel", "execution.release",
      "memory.regions", "memory.read", "n64.memory.read", "n64.debug.registers", "n64.debug.lastStop", "n64.debug.breakList", "n64.debug.watchList",
      "n64.input.state", "n64.profiler.top", "n64.dma.log", "n64.trace.page", "n64.state.format", "n64.state.layout", "n64.state.read", "n64.state.release",
      "video.begin", "video.read", "video.release", "video.record.status", "video.record.read", "video.record.release", "audio.status", "audio.read", "audio.release"
    };
    return std::find(allowed.begin(), allowed.end(), method) != allowed.end() ? std::string{} : "busy";
  };
  host.service.n64Request = [&](const std::string& method, const std::vector<std::string>& args, const std::string& owner) {
    if(method.starts_with("n64.state.load")) {
      if(method == "n64.state.loadCommit" && host.stateLoader.upload && host.takeoverRequested.load()) {
        host.stateLoader.reset(); if(!host.injectedInput.takeover()) ++host.injectedInput.takeovers;
        ares::Nintendo64::cpu.debugControl.resume(); host.service.paused = false;
        return ControlResult{"state_load_cancelled", {}};
      }
      auto savedInput = host.injectedInput.pending; auto oldFrame = execution.frame;
      auto result = host.stateLoader.execute(method, args, owner, n64, bool(execution.active), host.service.paused, host.stateSnapshot);
      if(host.stateLoader.touched) {
        ++host.service.epoch;
        host.injectedInput.releaseAll(); host.injectedInput.operationRunning = false;
        traceService.reset(); profilerService.reset(); host.captures.reset(host.service.epoch);
        host.audioCapture.reset(); host.recording.reset(); host.stateSnapshot.reset();
        ares::Nintendo64::executionCoordinate.reset();
        ares::Nintendo64::executionCoordinate.atBoundary = false; // restored VI phase is not an observed boundary
        ares::Nintendo64::dmaProvenance.reset(host.service.epoch);
        ares::Nintendo64::cpu.debugControl.power();
        execution.frame = 0; execution.atBoundary = false; execution.finished.clear();
        host.output.clear();
        bool keep = result.error.empty() && args[2] == "1";
        if(keep) for(unsigned port = 0; port < 4; ++port) if(savedInput[port].owner == owner) {
          auto neutral = savedInput[port]; neutral.buttons = 0; neutral.x = neutral.y = 0; neutral.epoch = host.service.epoch;
          if(neutral.expiresFrame != UINT64_MAX) neutral.expiresFrame = neutral.expiresFrame > oldFrame ? neutral.expiresFrame - oldFrame : 0;
          host.injectedInput.pending[port] = neutral;
        }
        host.injectedInput.epoch = host.service.epoch;
        host.injectedInput.service(host.service.epoch, 0, InputService::Clock::now(), true);
        host.service.paused = !result.error.empty() || args[1] != "1";
        if(result.error.empty()) {
          result.json.pop_back();
          result.json += ",\"version\":" + controlQuote(ares::Nintendo64::system.stateVersion().data()) + ",\"epoch\":" + std::to_string(host.service.epoch) + ",\"frame\":0,\"at_frame_boundary\":false,\"pc\":" + controlQuote(N64Service::hex(ares::Nintendo64::cpu.pipeline.pc))
            + ",\"stopped\":" + (host.service.paused ? "true" : "false") + ",\"paused\":" + (host.service.paused ? "true" : "false") + ",\"running\":" + (host.service.paused ? "false" : "true") + ",\"input\":" + host.injectedInput.state() + "}";
        }
      }
      return result;
    }
    if(method == "n64.state.begin" || method == "n64.state.read" || method == "n64.state.release") return host.stateSnapshot.execute(method, args, owner, host.service.epoch, execution.frame, n64, bool(execution.active), host.service.paused);
    if(method == "n64.state.format" || method == "n64.state.layout") {
      if(!args.empty()) return ControlResult{"invalid_request", {}};
      if(method == "n64.state.layout" && !n64) return ControlResult{"unavailable", {}};
      auto& system = ares::Nintendo64::system;
      auto json = std::string("{\"version\":") + controlQuote(system.stateVersion().data()) + ",\"signature\":" + std::to_string(ares::SerializerSignature) + ",\"header_size\":533";
      if(method == "n64.state.format") {
        json += ",\"slots\":[";
        if(emulator && emulator->game) for(unsigned slot = 1; slot <= 9; ++slot) {
          if(slot > 1) json += ',';
          // Match legacy slot names without locate()'s directory creation.
          auto location = emulator->game->location;
          string slotPath = settings.paths.saves ? string{settings.paths.saves, host.root->name(), "/", Location::prefix(location), ".bs", slot}
                                                : string{Location::notsuffix(location), ".bs", slot};
          json += controlQuote(slotPath.data());
        }
        json += ']';
      }
      if(method == "n64.state.layout") {
        auto [offset, size] = system.inspectStateRdramLayout();
        json += ",\"offset\":" + std::to_string(offset) + ",\"size\":" + std::to_string(size) + ",\"storage\":\"lsb32\"";
      }
      return ControlResult{{}, json + "}"};
    }
    if(method == "n64.input.tap") return tapService.execute(args, owner, host.service.epoch, host.service.paused, host.injectedInput, execution);
    // Keep the tapped port stable for its entire owned interval.
    if(tapService.active() && (method == "n64.input.set" || method == "n64.input.release") && !args.empty() &&
       (args[0] == "0" || args[0] == std::to_string(tapService.port))) return ControlResult{"busy", {}};
    if(method.starts_with("n64.profiler.")) return profilerService.execute(method, args, owner, bool(execution.active));
    if(method == "n64.dma.log") return dmaService.execute(args);
    if(method.starts_with("n64.trace.")) return traceService.execute(method, args, owner, host.service.epoch, host.service.paused, execution);
    if(method.starts_with("n64.input.")) return host.injectedInput.execute(method, args, owner, host.service.epoch, execution.frame, execution.active ? execution.active->owner : "");
    auto result = n64Service.execute(method, args, host.service.epoch, bool(execution.active));
    if(n64) execution.executor = N64Service::executor();
    return result;
  };
  if(n64) {
    auto& debug = ares::Nintendo64::cpu.debugControl;
    host.service.onPause = [&] { host.stateLoader.reset(); };
    host.service.onResume = [&](bool keepOverrides, const std::string& owner) {
      host.stateLoader.reset();
      if(!keepOverrides) {
        if(owner == "ui") host.injectedInput.releaseAll();
        else host.injectedInput.releaseOwner(owner);
      }
      debug.resume(); execution.executor = N64Service::executor();
    };
    execution.onStart = [&] {
      host.injectedInput.start(host.service.epoch, execution.frame);
      debug.resume();
      if(execution.instructionCount) debug.requestStepInstructions(execution.instructionCount);
      execution.executor = N64Service::executor();
    };
    execution.onFinish = [&](const std::string& reason) { traceService.finish(reason); tapService.finish(reason, host.injectedInput, host.service.epoch, execution.frame); host.injectedInput.finish(host.service.epoch, execution.frame); debug.stepping = false; debug.stepRemaining = 0; debug.updateActive(); };
    execution.debugSnapshot = [&] { return debug.halted && execution.active && execution.active->slices ? N64Service::stop(host.service.epoch) : "{\"valid\":false}"; };
  }
  execution.supported = synthetic || n64;
  execution.frameKind = synthetic ? "synthetic-vi" : n64 ? "vi" : "unavailable";
  execution.executor = synthetic ? "synthetic" : n64 ? (ares::Nintendo64::Accuracy::CPU::Recompiler && ares::Nintendo64::cpu.recompiler.enabled ? "recompiler" : "interpreter") : "unavailable";
  ares::Nintendo64::executionCoordinate.reset();
  // Yield even during ordinary N64 play so UI control stays serviceable if VI is
  // inactive. This is an outer CPU-loop budget, not an instruction-step count.
  if(n64) ares::Nintendo64::executionCoordinate.sliceBudget = 20000;
  while(!host.quit) {
    if(n64) {
      execution.frame = ares::Nintendo64::executionCoordinate.frame;
      execution.atBoundary = ares::Nintendo64::executionCoordinate.atBoundary;
      execution.supported = !nall::GDB::server.hasClient();
      execution.instructions = ares::Nintendo64::cpu.debugControl.instructionsRetired;
      execution.executor = N64Service::executor();
    }
    if(n64) host.injectedInput.service(host.service.epoch, execution.frame);
    bool wasControlled = bool(execution.active);
    if(!host.service.dispatch(synthetic ? "Synthetic" : std::string(host.systemName.data()), synthetic, [&] {
      host.injectedInput.releaseAll(); traceService.reset(); profilerService.reset();
      host.captures.reset(host.service.epoch + 1);
      host.audioCapture.reset();
      host.recording.reset();
      host.stateSnapshot.reset();
      host.stateLoader.reset();
      ares::Nintendo64::executionCoordinate.reset(); syntheticPhase = 0;
      ares::Nintendo64::dmaProvenance.reset(host.service.epoch + 1);
      if(host.root) { host.output.clear(); host.root->power(); }
    }, host.memory)) { host.quit = true; break; }
    bool takeover = host.takeoverRequested.exchange(false);
    bool loadTakeover = takeover && n64 && host.stateLoader.upload.has_value();
    if(loadTakeover) host.stateLoader.reset();
    bool inputTakeover = takeover && n64 && host.injectedInput.takeover();
    if(loadTakeover && !inputTakeover) ++host.injectedInput.takeovers;
    if(loadTakeover || inputTakeover) {
      execution.complete("host_input", host.service.paused);
      ares::Nintendo64::cpu.debugControl.resume();
      host.service.paused = false;
    }
    if(!wasControlled && execution.active && host.sound) host.output.clear();
    if(host.service.paused && !execution.active) {
      if(host.sound && !synthetic) host.output.clear();
      host.service.waitForRequest(std::chrono::milliseconds(10));
      deadline = Clock::now(); continue;
    }
    bool controlled = bool(execution.active);
    auto frameBefore = execution.frame;
    if(synthetic) {
      // Two deterministic slices per synthetic VI: the first ends mid-frame.
      execution.atBoundary = (++syntheticPhase == 2);
      if(!execution.atBoundary) { if(execution.active) ++execution.active->slices; continue; }
      syntheticPhase = 0; ++execution.frame;
      host.appliedInput = host.controls.load() >> 32;
      for(unsigned y = 0; y < height; y++) for(unsigned x = 0; x < width; x++) {
        pixels[y * (width + 8) + x] = 0xff000000u | ((x + tick) % 256 << 16) | ((y + tick) % 256 << 8) | (tick % 256);
      }
      host.video({}, pixels.data(), (width + 8) * 4, width, height); ++tick;
    } else { host.root->run(); profilerService.afterRun(); }
    if(execution.active) ++execution.active->slices;
    if(n64) {
      execution.frame = ares::Nintendo64::executionCoordinate.frame;
      execution.atBoundary = ares::Nintendo64::executionCoordinate.atBoundary;
    }
    if(n64) {
      auto& debug = ares::Nintendo64::cpu.debugControl;
      execution.instructions = debug.instructionsRetired;
      execution.executor = N64Service::executor();
      if(debug.halted) { execution.complete(N64Service::reason(debug.stop.reason), host.service.paused); host.service.paused = true; }
    }
    execution.service(host.service.paused);
    if(controlled) { deadline = Clock::now(); continue; }
    if((synthetic || !host.sound) && (!n64 || execution.frame != frameBefore || ares::Nintendo64::executionCoordinate.livenessYield)) {
      deadline += std::chrono::nanoseconds(synthetic ? 16666667 : int64_t(1e9 / host.refreshRate.load()));
      std::this_thread::sleep_until(deadline);
      if(Clock::now() - deadline > std::chrono::milliseconds(100)) deadline = Clock::now();
    }
  }
  execution.complete("shutdown", host.service.paused);
  host.quit = true; host.wake.notify_all();
  host.service.close();
  writer.join(); reader.join();
  if(replies.joinable()) replies.join();
  if(host.root) {
    emulator->unload(); host.streams.clear(); host.root.reset(); emulator.reset();
  }
  ares::Video::frameSubmittedObserver = {};
  std::cerr << "stopped\n";
  ares::platform = nullptr;
}
