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
  nall::string firmwarePath;
  nall::string savesPath;
  nall::string settingsPath;
};

auto parseLaunchSettings(nall::Arguments& arguments, LaunchSettings& settings, nall::string& error) -> bool;
auto lookupSettingFileValue(const LaunchSettings& settings, const nall::string& path) -> nall::string;
auto lookupFirmwareLocation(
  const LaunchSettings& settings,
  const nall::string& systemName,
  const nall::string& type,
  const nall::string& region
) -> nall::string;

}
