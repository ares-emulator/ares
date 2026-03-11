#include <ares/ares.hpp>
#include "emulators.hpp"
#include "settings.hpp"
#include <mia/mia.hpp>
#include <nall/encode/png.hpp>
#include <nall/gdb/server.hpp>
#include <nall/main.hpp>
#include <limits>
#define XXH_INLINE_ALL
#include <thirdparty/xxhash.h>

using namespace nall;

namespace {

auto parsePositiveInteger(const string& value, u64& out) -> bool {
  if(value.length() == 0) return false;
  u64 parsed = 0;
  for(auto ch : value) {
    if(ch < '0' || ch > '9') return false;
    u64 digit = ch - '0';
    if(parsed > (std::numeric_limits<u64>::max() - digit) / 10) return false;
    parsed = parsed * 10 + digit;
  }
  if(parsed == 0) return false;
  out = parsed;
  return true;
}

auto parseBenchmarkSpec(const string& value, double& durationSeconds, u64& frameTarget) -> bool {
  durationSeconds = 0.0;
  frameTarget = 0;
  if(value.length() == 0) return false;

  auto suffix = value[value.size() - 1];
  if(suffix == 's' || suffix == 'S' || suffix == 'f' || suffix == 'F') {
    string prefix;
    for(u32 i : range((u32)value.size() - 1)) prefix.append(value[i]);
    u64 parsed = 0;
    if(!parsePositiveInteger(prefix, parsed)) return false;
    if(suffix == 'f' || suffix == 'F') frameTarget = parsed;
    else durationSeconds = (double)parsed;
    return true;
  }

  durationSeconds = value.real();
  return durationSeconds > 0.0;
}

auto parseVerbosity(const string& value, u32& out) -> bool {
  if(value == "0" || value == "quiet") {
    out = 0;
    return true;
  }
  if(value == "1" || value == "normal") {
    out = 1;
    return true;
  }
  if(value == "2" || value == "verbose") {
    out = 2;
    return true;
  }
  return false;
}

auto parseStateSlot(const string& value, u32& out) -> bool {
  if(!value) {
    out = 1;
    return true;
  }

  u64 parsed = 0;
  if(!parsePositiveInteger(value, parsed) || parsed < 1 || parsed > 9) return false;
  out = (u32)parsed;
  return true;
}

auto locatePathForSave(const string& location, const string& suffix, const string& path, const string& system) -> string {
  if(!path) return {Location::notsuffix(location), suffix};

  string pathname = {path, system, "/"};
  directory::create(pathname);
  return {pathname, Location::prefix(location), suffix};
}

auto loadSaveState(ares::Node::System& root, const string& gameLocation, u32 slot, const string& savesPath) -> bool {
  auto location = locatePathForSave(gameLocation, {".bs", slot}, savesPath, root->name());
  auto memory = file::read(location);
  if(memory.empty()) return false;

  serializer state{memory.data(), (u32)memory.size()};
  return root->unserialize(state);
}

auto saveStateToSlot(ares::Node::System& root, const string& gameLocation, u32 slot, const string& savesPath) -> bool {
  auto location = locatePathForSave(gameLocation, {".bs", slot}, savesPath, root->name());
  if(auto state = root->serialize()) {
    return file::write(location, {state.data(), state.size()});
  }
  return false;
}

auto resampleNearest(
  const std::vector<u32>& source,
  u32 sourceWidth,
  u32 sourceHeight,
  u32 sourcePitch,
  u32 targetWidth,
  u32 targetHeight
) -> std::vector<u32> {
  std::vector<u32> output;
  if(!sourceWidth || !sourceHeight || !targetWidth || !targetHeight) return output;

  output.resize((size_t)targetWidth * targetHeight);
  auto sourceStride = sourcePitch >> 2;
  for(u32 y : range(targetHeight)) {
    auto sy = (u64)y * sourceHeight / targetHeight;
    for(u32 x : range(targetWidth)) {
      auto sx = (u64)x * sourceWidth / targetWidth;
      output[(size_t)y * targetWidth + x] = source[(size_t)sy * sourceStride + sx];
    }
  }
  return output;
}

struct Runtime : ares::Platform {
  ares::Node::System root;
  std::shared_ptr<mia::Pak> systemPak;
  std::shared_ptr<mia::Pak> gamePak;
  string medium;
  string profile;

  std::vector<ares::Node::Video::Screen> screens;

  u64 runFramesTarget = 0;
  u32 saveStateSlot = 0;
  bool gdbEnabled = false;
  u32 gdbPort = 9123;
  bool gdbUseIPv4 = false;
  bool awaitGdbClient = false;
  u32 verbosity = 1;
  u64 runFramesCounter = 0;
  bool videoChecksum = false;
  double benchmarkDuration = 0.0;
  u64 benchmarkFrameTarget = 0;
  u64 benchmarkFrameCount = 0;
  u64 benchmarkStartTime = 0;
  bool shouldExit = false;
  string saveLastFramePath;
  std::vector<u32> lastFrame;
  u32 lastFramePitch = 0;
  u32 lastFrameWidth = 0;
  u32 lastFrameHeight = 0;

  auto attach(ares::Node::Object node) -> void override {
    if(node->cast<ares::Node::Video::Screen>()) {
      screens = root->find<ares::Node::Video::Screen>();
    }
  }

  auto detach(ares::Node::Object node) -> void override {
    if(auto screen = node->cast<ares::Node::Video::Screen>()) {
      screens = root->find<ares::Node::Video::Screen>();
      std::erase(screens, screen);
    }
  }

  auto pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> override {
    if(!systemPak || !gamePak) return {};
    auto name = node->name();

    if(name == medium || name == profile) return systemPak->pak;
    if(name.find("Cartridge") || name.find("Disc") || name.find("Disk") || name.find("Tape") || name.find("Cassette")) return gamePak->pak;
    if(name.find("Controller Pak") || name.find("Memory Card")) return systemPak->pak;

    // Fallback: prefer game data for unknown leaf paks.
    return gamePak->pak;
  }

  auto status(string_view message) -> void override {
    if(verbosity == 0) return;
    fprintf(stderr, "%s\n", message.data());
  }

  auto video(ares::Node::Video::Screen node, const u32* data, u32 pitch, u32 width, u32 height) -> void override {
    if(shouldExit) return;

    if(saveLastFramePath && !screens.empty() && node == screens[0]) {
      lastFramePitch = pitch;
      lastFrameHeight = height;
      lastFrameWidth = width;
      auto pixels = (size_t)(pitch >> 2) * height;
      lastFrame.resize(pixels);
      memory::copy<u32>(lastFrame.data(), data, pixels);
    }

    if(!screens.empty() && node == screens[0] && runFramesTarget > 0) {
      runFramesCounter++;
      if(runFramesCounter >= runFramesTarget) {
        if(videoChecksum) {
          auto checksum = XXH64(data, (size_t)pitch * height, 0);
          print(hex(checksum, 16L), "\n");
        }
        shouldExit = true;
      }
    }

    if(benchmarkDuration > 0.0 || benchmarkFrameTarget > 0) {
      if(benchmarkStartTime == 0) benchmarkStartTime = chrono::nanosecond();
      benchmarkFrameCount++;

      u64 current = chrono::nanosecond();
      bool reachedTime = benchmarkDuration > 0.0 && current - benchmarkStartTime >= benchmarkDuration * 1'000'000'000.0;
      bool reachedFrames = benchmarkFrameTarget > 0 && benchmarkFrameCount >= benchmarkFrameTarget;
      if(reachedTime || reachedFrames) {
        string mode = benchmarkFrameTarget > 0 ? "frames" : "seconds";
        double elapsed = (current - benchmarkStartTime) / 1'000'000'000.0;
        double fps = benchmarkFrameCount / elapsed;
        print("{\n");
        print("  \"core\": \"", medium, "\",\n");
        print("  \"game\": \"", Location::file(gamePak->location), "\",\n");
        print("  \"benchmark_mode\": \"", mode, "\",\n");
        print("  \"duration_seconds\": ", elapsed, ",\n");
        print("  \"frames_rendered\": ", benchmarkFrameCount, ",\n");
        print("  \"average_fps\": ", fps, "\n");
        print("}\n");
        shouldExit = true;
      }
    }
  }

  auto audio(ares::Node::Audio::Stream node) -> void override {
    while(node->pending()) {
      f64 buffer[2];
      node->read(buffer);
    }
  }

  auto input(ares::Node::Input::Input node) -> void override {
    if(auto button = node->cast<ares::Node::Input::Button>()) button->setValue(0);
    if(auto axis = node->cast<ares::Node::Input::Axis>()) axis->setValue(0);
    if(auto trigger = node->cast<ares::Node::Input::Trigger>()) trigger->setValue(0);
  }
};

}

auto nall::main(Arguments arguments) -> void {
  ares::Memory::FixedAllocator::get();

  string systemOverride;
  string gamePath;
  double benchmarkDuration = 0.0;
  u64 benchmarkFrameTarget = 0;
  u64 runFramesTarget = 0;
  u32 saveStateSlot = 0;
  bool saveOnExit = false;
  u32 saveOnExitSlot = 1;
  u32 verbosity = 1;
  string saveLastFramePath;
  bool videoChecksum = false;
  headless::LaunchSettings launchSettings;

  while(arguments.find("--system")) arguments.take("--system", systemOverride);
  {
    string settingError;
    if(!headless::parseLaunchSettings(arguments, launchSettings, settingError)) {
      fprintf(stderr, "error: %s\n", settingError.data());
      return;
    }
  }
  if(string benchmarkSpec; arguments.take("--benchmark", benchmarkSpec)) {
    if(!parseBenchmarkSpec(benchmarkSpec, benchmarkDuration, benchmarkFrameTarget)) {
      fprintf(stderr, "error: --benchmark expects a positive value (eg: 5, 5s, 300f).\n");
      return;
    }
  }
  if(arguments.find("--run-frames")) {
    string frames;
    if(!arguments.take("--run-frames", frames) || !parsePositiveInteger(frames, runFramesTarget)) {
      fprintf(stderr, "error: --run-frames requires a positive integer frame count.\n");
      return;
    }
  }
  {
    string saveState;
    while(arguments.takeOptional("--save-state", saveState)) {
      if(!parseStateSlot(saveState, saveStateSlot)) {
        fprintf(stderr, "error: --save-state accepts optional slot 1-9.\n");
        return;
      }
    }
  }
  if(arguments.take("--quiet")) verbosity = 0;
  if(arguments.take("--verbose")) verbosity = 2;
  if(arguments.find("--verbosity")) {
    string verbosityMode;
    if(!arguments.take("--verbosity", verbosityMode) || !parseVerbosity(verbosityMode, verbosity)) {
      fprintf(stderr, "error: --verbosity expects one of: quiet|normal|verbose (or 0|1|2).\n");
      return;
    }
  }
  {
    string saveOnExitValue;
    while(arguments.takeOptional("--save-on-exit", saveOnExitValue)) {
      if(!parseStateSlot(saveOnExitValue, saveOnExitSlot)) {
        fprintf(stderr, "error: --save-on-exit accepts optional slot 1-9.\n");
        return;
      }
      saveOnExit = true;
    }
  }
  if(arguments.find("--save-last-frame") && !arguments.take("--save-last-frame", saveLastFramePath)) {
    fprintf(stderr, "error: --save-last-frame requires an output .png path.\n");
    return;
  }
  videoChecksum = arguments.take("--video-checksum");

  if(arguments.take("--help")) {
    print("\n Usage: ares-headless [OPTIONS]... game\n\n");
    print("Options:\n");
    print("  --help                Displays available options and exit\n");
    print("  --version             Displays the version string of the application\n");
    print("  --system name         Override auto-detected system\n");
    print("  --setting name=value  Override supported core settings\n");
    print("  --benchmark value     Run benchmark for N seconds (eg 5s / 5) or N frames (eg 300f)\n");
    print("  --run-frames frames   Run for exactly N frames and then exit\n");
    print("  --quiet               Suppress non-essential runtime logging\n");
    print("  --verbose             Alias for --verbosity verbose\n");
    print("  --verbosity level     quiet|normal|verbose (or 0|1|2)\n");
    print("  --video-checksum      Print the final frame buffer checksum on exit\n");
    print("  --save-state slot     Load save state slot (default: 1)\n");
    print("  --save-on-exit slot   Save state on exit (default: 1)\n");
    print("  --save-last-frame p   Save final rendered frame to PNG path\n");
    print("  --settings-file path  Reserved for compatibility (ignored)\n");
    return;
  }
  if(arguments.take("--version")) {
    print(ares::Version, "\n");
    return;
  }

  for(auto arg : arguments) {
    if(!arg.beginsWith("--")) {
      gamePath = arg;
      break;
    }
  }
  if(!gamePath || !inode::exists(gamePath)) {
    fprintf(stderr, "error: provide a valid game file or directory.\n");
    return;
  }

  mia::setHomeLocation([]() -> string { return {Path::userData(), "ares/Systems/"}; });
  mia::setSaveLocation([&launchSettings]() -> string { return launchSettings.savesPath; });

  Runtime runtime;
  runtime.runFramesTarget = runFramesTarget;
  runtime.saveStateSlot = saveStateSlot;
  runtime.gdbEnabled = launchSettings.gdbEnabled;
  runtime.gdbPort = launchSettings.gdbPort;
  runtime.gdbUseIPv4 = launchSettings.gdbUseIPv4;
  runtime.awaitGdbClient = launchSettings.awaitGdbClient;
  runtime.verbosity = verbosity;
  runtime.saveLastFramePath = saveLastFramePath;
  runtime.videoChecksum = videoChecksum;
  runtime.benchmarkDuration = benchmarkDuration;
  runtime.benchmarkFrameTarget = benchmarkFrameTarget;

  runtime.medium = systemOverride ? systemOverride : mia::identify(gamePath);
  if(!runtime.medium) {
    fprintf(stderr, "error: unable to determine game type for: %s\n", Location::file(gamePath).data());
    return;
  }

  runtime.gamePak = mia::Medium::create(runtime.medium);
  if(!runtime.gamePak) {
    fprintf(stderr, "error: unsupported system: %s\n", runtime.medium.data());
    return;
  }
  auto gameLoad = runtime.gamePak->load(gamePath);
  if(gameLoad != successful) {
    fprintf(stderr, "error: failed to load game media.\n");
    return;
  }

  auto systemName = headless::defaultSystemNameForMedium(runtime.medium);

  runtime.systemPak = mia::System::create(systemName);
  if(!runtime.systemPak || runtime.systemPak->load() != successful) {
    fprintf(stderr, "error: failed to load system data for: %s\n", systemName.data());
    return;
  }

  auto region = runtime.gamePak->pak ? headless::normalizeRegion(runtime.gamePak->pak->attribute("region")) : string{};
  runtime.profile = headless::defaultProfileForMedium(runtime.medium, region, launchSettings.coreOptions.gameBoyAdvancePlayer);
  if(!runtime.profile) {
    fprintf(stderr, "error: no default core profile for system: %s\n", runtime.medium.data());
    return;
  }

  ares::platform = &runtime;
  headless::configureCoreOptionsForMedium(runtime.medium, launchSettings.coreOptions);
  if(!headless::loadCoreForMedium(runtime.medium, runtime.root, runtime.profile)) {
    fprintf(stderr, "error: failed to load ares core profile: %s\n", runtime.profile.data());
    return;
  }

  headless::connectDefaultPorts(runtime.root);

  if(runtime.gdbEnabled) {
    nall::GDB::server.reset();
    nall::GDB::server.open(runtime.gdbPort, runtime.gdbUseIPv4);
    nall::GDB::server.onClientConnectCallback = []() {
      fprintf(stderr, "GDB client connected\n");
    };

    if(runtime.awaitGdbClient) {
      fprintf(stderr, "Waiting for GDB client on port %u...\n", runtime.gdbPort);
      while(!runtime.shouldExit && !nall::GDB::server.hasClient()) {
        nall::GDB::server.updateLoop();
        usleep(20 * 1000);
      }
    }
  }

  if(verbosity == 0) {
    freopen("/dev/null", "w", stderr);
  }

  runtime.root->power();
  if(runtime.saveStateSlot) {
    if(!loadSaveState(runtime.root, runtime.gamePak->location, runtime.saveStateSlot, launchSettings.savesPath)) {
      fprintf(stderr, "warning: failed to load state from slot %u\n", runtime.saveStateSlot);
    } else {
      if(verbosity >= 1) print("Loaded state from slot ", runtime.saveStateSlot, "\n");
    }
  }
  while(!runtime.shouldExit) {
    if(runtime.gdbEnabled && nall::GDB::server.isHalted()) {
      nall::GDB::server.updateLoop();
      usleep(1000);
      continue;
    }
    if(runtime.gdbEnabled) {
      nall::GDB::server.updateLoop();
    }
    runtime.root->run();
    if(runtime.gdbEnabled) {
      nall::GDB::server.updateLoop();
    }
  }

  if(saveOnExit) {
    if(saveStateToSlot(runtime.root, runtime.gamePak->location, saveOnExitSlot, launchSettings.savesPath)) {
      if(verbosity >= 1) print("Saved state to slot ", saveOnExitSlot, "\n");
    } else {
      fprintf(stderr, "warning: failed to save state to slot %u\n", saveOnExitSlot);
    }
  }

  runtime.root->save();
  if(runtime.saveLastFramePath) {
    if(runtime.lastFrame.empty() || runtime.lastFramePitch == 0 || runtime.lastFrameHeight == 0) {
      fprintf(stderr, "warning: no video frame captured, PNG not written.\n");
    } else {
      u32 targetWidth = runtime.lastFrameWidth;
      u32 targetHeight = runtime.lastFrameHeight;
      if(!runtime.screens.empty() && runtime.screens[0]) {
        targetWidth = runtime.screens[0]->width() * runtime.screens[0]->scaleX();
        targetHeight = runtime.screens[0]->height() * runtime.screens[0]->scaleY();
        if(!targetWidth) targetWidth = runtime.lastFrameWidth;
        if(!targetHeight) targetHeight = runtime.lastFrameHeight;
      }

      const u32* pngData = runtime.lastFrame.data();
      u32 pngPitch = runtime.lastFramePitch;
      u32 pngWidth = runtime.lastFrameWidth;
      u32 pngHeight = runtime.lastFrameHeight;

      auto resized = std::vector<u32>{};
      if(targetWidth != runtime.lastFrameWidth || targetHeight != runtime.lastFrameHeight) {
        resized = resampleNearest(
          runtime.lastFrame,
          runtime.lastFrameWidth,
          runtime.lastFrameHeight,
          runtime.lastFramePitch,
          targetWidth,
          targetHeight
        );
        if(!resized.empty()) {
          pngData = resized.data();
          pngPitch = targetWidth * sizeof(u32);
          pngWidth = targetWidth;
          pngHeight = targetHeight;
        }
      }

      if(!Encode::PNG::RGB8(runtime.saveLastFramePath, pngData, pngPitch, pngWidth, pngHeight)) {
        fprintf(stderr, "warning: failed to write PNG: %s\n", runtime.saveLastFramePath.data());
      }
    }
  }
  runtime.gamePak->save(runtime.gamePak->location);
  runtime.systemPak->save(runtime.systemPak->location);
  runtime.root->unload();
  if(runtime.gdbEnabled) {
    nall::GDB::server.close();
    nall::GDB::server.reset();
  }
}
