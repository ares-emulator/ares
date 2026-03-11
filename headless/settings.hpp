#pragma once

#include "emulators.hpp"
#include <nall/arguments.hpp>
#include <cstdint>

namespace headless {

struct LaunchSettings {
  CoreOptions coreOptions;
  bool gdbEnabled = false;
  std::uint32_t gdbPort = 9123;
  bool gdbUseIPv4 = false;
  bool awaitGdbClient = false;
  nall::string savesPath;
  nall::string settingsPath;
};

auto parseLaunchSettings(nall::Arguments& arguments, LaunchSettings& settings, nall::string& error) -> bool;

}
