#include "runtime.hpp"
#include <nall/location.hpp>
#define XXH_INLINE_ALL
#include <thirdparty/xxhash.h>

using namespace nall;

namespace headless {

auto Runtime::attach(ares::Node::Object node) -> void {
  if(node->cast<ares::Node::Video::Screen>()) {
    screens = root->find<ares::Node::Video::Screen>();
  }
}

auto Runtime::detach(ares::Node::Object node) -> void {
  if(auto screen = node->cast<ares::Node::Video::Screen>()) {
    screens = root->find<ares::Node::Video::Screen>();
    std::erase(screens, screen);
  }
}

auto Runtime::pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> {
  if(!systemPak || !gamePak) return {};
  auto name = node->name();

  if(name == medium || name == profile) return systemPak->pak;
  if(name.find("Cartridge") || name.find("Disc") || name.find("Disk") || name.find("Tape") || name.find("Cassette")) return gamePak->pak;
  if(name.find("Controller Pak") || name.find("Memory Card")) return systemPak->pak;

  // Fallback: prefer game data for unknown leaf paks.
  return gamePak->pak;
}

auto Runtime::status(string_view message) -> void {
  if(verbosity == 0) return;
  fprintf(stderr, "%s\n", message.data());
}

auto Runtime::video(ares::Node::Video::Screen node, const u32* data, u32 pitch, u32 width, u32 height) -> void {
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

auto Runtime::audio(ares::Node::Audio::Stream node) -> void {
  while(node->pending()) {
    f64 buffer[2];
    node->read(buffer);
  }
}

auto Runtime::input(ares::Node::Input::Input node) -> void {
  if(auto button = node->cast<ares::Node::Input::Button>()) button->setValue(0);
  if(auto axis = node->cast<ares::Node::Input::Axis>()) axis->setValue(0);
  if(auto trigger = node->cast<ares::Node::Input::Trigger>()) trigger->setValue(0);
}

}
