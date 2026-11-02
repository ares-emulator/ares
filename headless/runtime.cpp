#include "runtime.hpp"
#include <algorithm>
#include <nall/file.hpp>
#include <nall/location.hpp>
#define XXH_INLINE_ALL
#include <thirdparty/xxhash.h>

using namespace nall;

namespace headless {

namespace {

auto nodeOrderKey(ares::Node::Object node) -> string {
  if(!node) return {};

  // Build a stable tree path so audio/video node ordering does not depend on
  // attach timing or container traversal details when we enumerate nodes.
  string path;
  if(auto parent = node->parent().lock()) {
    path = nodeOrderKey(parent);

    u32 siblingIndex = 0;
    for(auto& sibling : *parent) {
      if(sibling == node) break;
      if(sibling->identity() == node->identity() && sibling->name() == node->name()) siblingIndex++;
    }

    path.append("/");
    path.append(node->identity());
    path.append(":");
    path.append(node->name());
    path.append("#");
    path.append(siblingIndex);
    return path;
  }

  path.append(node->identity());
  path.append(":");
  path.append(node->name());
  path.append("#0");
  return path;
}

template<typename T>
auto sortNodes(std::vector<T>& nodes) -> void {
  std::sort(nodes.begin(), nodes.end(), [](const T& lhs, const T& rhs) {
    auto lhsKey = nodeOrderKey(lhs);
    auto rhsKey = nodeOrderKey(rhs);
    return lhsKey < rhsKey;
  });
}

}

auto Runtime::attach(ares::Node::Object node) -> void {
  if(node->cast<ares::Node::Video::Screen>()) {
    screens = root->find<ares::Node::Video::Screen>();
    sortNodes(screens);
  }

  if(auto stream = node->cast<ares::Node::Audio::Stream>()) {
    streams = root->find<ares::Node::Audio::Stream>();
    sortNodes(streams);
    for(auto& stream : streams) {
      stream->setResamplerFrequency(audioFrequency);
    }
  }
}

auto Runtime::detach(ares::Node::Object node) -> void {
  if(auto screen = node->cast<ares::Node::Video::Screen>()) {
    screens = root->find<ares::Node::Video::Screen>();
    std::erase(screens, screen);
    sortNodes(screens);
  }

  if(auto stream = node->cast<ares::Node::Audio::Stream>()) {
    streams = root->find<ares::Node::Audio::Stream>();
    std::erase(streams, stream);
    sortNodes(streams);
  }
}

auto Runtime::pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> {
  if(!systemPak || !gamePak) return {};
  auto name = node->name();

  if(
    medium == "Mega Drive" || medium == "Mega 32X" || medium == "Mega CD" ||
    medium == "Mega LD" || medium == "Mega CD 32X"
  ) {
    // Mega Drive family cores ask for the base console pak as "Mega Drive",
    // even when the loaded medium is 32X/CD-backed. Route those names the
    // same way as desktop-ui so the core sees system ROMs instead of the cart.
    if(name == "Mega Drive") return systemPak->pak;
    if(name == "Mega Drive Cartridge") return gamePak->pak;
    if(name == "Mega CD Disc") return gamePak->pak;
  }

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
      stopRequested = true;
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
      stopRequested = true;
    }
  }
}

auto Runtime::audio(ares::Node::Audio::Stream node) -> void {
  (void)node;
  mixPendingAudioFrames();
}

auto Runtime::input(ares::Node::Input::Input node) -> void {
  if(auto button = node->cast<ares::Node::Input::Button>()) button->setValue(0);
  if(auto axis = node->cast<ares::Node::Input::Axis>()) axis->setValue(0);
  if(auto trigger = node->cast<ares::Node::Input::Trigger>()) trigger->setValue(0);
}

auto Runtime::flushPendingAudio() -> void {
  mixPendingAudioFrames();
}

auto Runtime::mixPendingAudioFrames() -> void {
  if(streams.empty()) return;

  while(true) {
    // Match the desktop frontend's behavior: only mix when every stream has at
    // least one resampled frame ready, so multi-stream systems stay aligned.
    for(auto& stream : streams) {
      if(!stream->pending()) return;
    }

    f64 samples[2] = {0.0, 0.0};
    for(auto& stream : streams) {
      f64 buffer[2] = {0.0, 0.0};
      u32 channels = stream->read(buffer);
      if(channels <= 1) {
        samples[0] += buffer[0];
        samples[1] += buffer[0];
      } else {
        samples[0] += buffer[0];
        samples[1] += buffer[1];
      }
    }

    appendMixedAudioFrame(samples);
  }
}

auto Runtime::appendMixedAudioFrame(const f64 samples[2]) -> void {
  i16 pcm[2];
  // Canonicalize the captured output as clamped stereo 16-bit PCM so hashes and
  // WAV dumps operate on the exact same sample representation.
  for(u32 channel : range(2)) {
    auto sample = max(-1.0, min(+1.0, samples[channel]));
    pcm[channel] = sclamp<16>(sample * 32767.0);
  }

  if(audioDumpPath || audioChecksum) {
    audioSamples.push_back(pcm[0]);
    audioSamples.push_back(pcm[1]);
  }
}

auto Runtime::finalizeAudioCapture() -> void {
  if(audioChecksum) {
    auto checksum = XXH64(audioSamples.data(), audioSamples.size() * sizeof(i16), 0);
    print(hex(checksum, 16L), "\n");
  }
}

}
