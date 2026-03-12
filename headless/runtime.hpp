#pragma once

#include <ares/ares.hpp>
#include <mia/mia.hpp>
#include <vector>

namespace headless {

struct Runtime : ares::Platform {
  ares::Node::System root;
  std::shared_ptr<mia::Pak> systemPak;
  std::shared_ptr<mia::Pak> gamePak;
  nall::string medium;
  nall::string profile;

  std::vector<ares::Node::Video::Screen> screens;

  u64 runFramesTarget = 0;
  u32 stateSlot = 0;
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
  nall::string saveLastFramePath;
  std::vector<u32> lastFrame;
  u32 lastFramePitch = 0;
  u32 lastFrameWidth = 0;
  u32 lastFrameHeight = 0;

  auto attach(ares::Node::Object node) -> void override;
  auto detach(ares::Node::Object node) -> void override;
  auto pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> override;
  auto status(nall::string_view message) -> void override;
  auto video(ares::Node::Video::Screen node, const u32* data, u32 pitch, u32 width, u32 height) -> void override;
  auto audio(ares::Node::Audio::Stream node) -> void override;
  auto input(ares::Node::Input::Input node) -> void override;
};

}
