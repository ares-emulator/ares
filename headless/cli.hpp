#pragma once

#include "settings.hpp"
#include <nall/arguments.hpp>

namespace headless {

struct CliOptions {
  nall::string systemOverride;
  nall::string gamePath;
  double benchmarkDuration = 0.0;
  u64 benchmarkFrameTarget = 0;
  u64 runFramesTarget = 0;
  u32 stateSlot = 0;
  bool saveOnExit = false;
  u32 saveOnExitSlot = 1;
  u32 verbosity = 1;
  nall::string saveLastFramePath;
  bool videoChecksum = false;
  bool showHelp = false;
  bool showVersion = false;
  LaunchSettings launchSettings;
};

auto parseCliOptions(nall::Arguments& arguments, CliOptions& options, nall::string& error) -> bool;
auto printHeadlessUsage() -> void;

}
