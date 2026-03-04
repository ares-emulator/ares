#include <ares/ares.hpp>
#include "emulator.hpp"
#include <mia/mia.hpp>
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

struct Runtime : ares::Platform {
  ares::Node::System root;
  std::shared_ptr<mia::Pak> systemPak;
  std::shared_ptr<mia::Pak> gamePak;
  string medium;
  string profile;

  std::vector<ares::Node::Video::Screen> screens;

  u64 runFramesTarget = 0;
  u64 runFramesCounter = 0;
  bool videoChecksum = false;
  double benchmarkDuration = 0.0;
  u64 benchmarkFrameTarget = 0;
  u64 benchmarkFrameCount = 0;
  u64 benchmarkStartTime = 0;
  bool shouldExit = false;

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
    fprintf(stderr, "%s\n", message.data());
  }

  auto video(ares::Node::Video::Screen node, const u32* data, u32 pitch, u32, u32 height) -> void override {
    if(shouldExit) return;

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
  string settingsPath;
  double benchmarkDuration = 0.0;
  u64 benchmarkFrameTarget = 0;
  u64 runFramesTarget = 0;
  bool videoChecksum = false;

  while(arguments.find("--system")) arguments.take("--system", systemOverride);
  while(arguments.find("--settings-file")) arguments.take("--settings-file", settingsPath);
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
  videoChecksum = arguments.take("--video-checksum");

  if(arguments.take("--help")) {
    print("\n Usage: ares-nogui [OPTIONS]... game\n\n");
    print("Options:\n");
    print("  --help                Displays available options and exit\n");
    print("  --version             Displays the version string of the application\n");
    print("  --system name         Override auto-detected system\n");
    print("  --benchmark value     Run benchmark for N seconds (eg 5s / 5) or N frames (eg 300f)\n");
    print("  --run-frames frames   Run for exactly N frames and then exit\n");
    print("  --video-checksum      Print the final frame buffer checksum on exit\n");
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
  mia::setSaveLocation([]() -> string { return {}; });

  Runtime runtime;
  runtime.runFramesTarget = runFramesTarget;
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

  auto systemName = nogui::defaultSystemNameForMedium(runtime.medium);

  runtime.systemPak = mia::System::create(systemName);
  if(!runtime.systemPak || runtime.systemPak->load() != successful) {
    fprintf(stderr, "error: failed to load system data for: %s\n", systemName.data());
    return;
  }

  auto region = runtime.gamePak->pak ? nogui::normalizeRegion(runtime.gamePak->pak->attribute("region")) : string{};
  runtime.profile = nogui::defaultProfileForMedium(runtime.medium, region);
  if(!runtime.profile) {
    fprintf(stderr, "error: no default core profile for system: %s\n", runtime.medium.data());
    return;
  }

  ares::platform = &runtime;
  if(!nogui::loadCoreForMedium(runtime.medium, runtime.root, runtime.profile)) {
    fprintf(stderr, "error: failed to load ares core profile: %s\n", runtime.profile.data());
    return;
  }

  nogui::connectDefaultPorts(runtime.root);

  runtime.root->power();
  while(!runtime.shouldExit) {
    runtime.root->run();
  }

  runtime.root->save();
  runtime.gamePak->save(runtime.gamePak->location);
  runtime.systemPak->save(runtime.systemPak->location);
  runtime.root->unload();
}
