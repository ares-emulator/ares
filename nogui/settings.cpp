#include "settings.hpp"
#include <limits>
#include <nall/file.hpp>
#include <nall/path.hpp>
#include <nall/string/markup/bml.hpp>

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

auto parseSettingValue(bool& out, const string& value) -> bool {
  out = value.boolean();
  return true;
}

auto parseSettingValue(string& out, const string& value) -> bool {
  out = value;
  return true;
}

auto parseSettingValue(std::uint32_t& out, const string& value) -> bool {
  u64 parsed = 0;
  if(!parsePositiveInteger(value, parsed) || parsed > std::numeric_limits<std::uint32_t>::max()) return false;
  out = (std::uint32_t)parsed;
  return true;
}

auto applySettingOverride(
  nogui::LaunchSettings& settings,
  const string& name,
  const string& value
) -> bool {
  #define bind(path, target) \
    if(name == path) { \
      if(!parseSettingValue(target, value)) return false; \
      return true; \
    }

  bind("Video/PixelAccuracy", settings.coreOptions.videoPixelAccuracy);
  bind("Video/Quality", settings.coreOptions.videoQuality);
  bind("Video/Supersampling", settings.coreOptions.videoSupersampling);
  bind("Video/DisableVideoInterfaceProcessing", settings.coreOptions.videoDisableVideoInterfaceProcessing);
  bind("Video/WeaveDeinterlacing", settings.coreOptions.videoWeaveDeinterlacing);

  bind("General/HomebrewMode", settings.coreOptions.generalHomebrewMode);
  bind("General/ForceInterpreter", settings.coreOptions.generalForceInterpreter);

  bind("Nintendo64/ExpansionPak", settings.coreOptions.nintendo64ExpansionPak);
  bind("Nintendo64/ControllerPakBankString", settings.coreOptions.nintendo64ControllerPakBankString);

  bind("GameBoyAdvance/Player", settings.coreOptions.gameBoyAdvancePlayer);
  bind("MegaDrive/TMSS", settings.coreOptions.megadriveTMSS);

  bind("DebugServer/Enabled", settings.gdbEnabled);
  bind("DebugServer/Port", settings.gdbPort);
  bind("DebugServer/UseIPv4", settings.gdbUseIPv4);
  bind("Paths/Saves", settings.savesPath);

  #undef bind

  if(name == "Boot/AwaitGDBClient") {
    settings.awaitGdbClient = value.boolean();
    if(settings.awaitGdbClient) settings.gdbEnabled = true;
    return true;
  }

  return false;
}

auto loadSettingsFileDefaults(nogui::LaunchSettings& settings) -> void {
  auto settingsFile = settings.settingsPath;
  if(!settingsFile) settingsFile = {Path::userData(), "ares/settings.bml"};
  if(!file::exists(settingsFile)) return;

  auto document = BML::unserialize(string::read(settingsFile), " ");
  if(auto node = document["Paths/Saves"]) settings.savesPath = node.string();
}

}

namespace nogui {

auto parseLaunchSettings(Arguments& arguments, LaunchSettings& settings, string& error) -> bool {
  while(arguments.find("--settings-file")) arguments.take("--settings-file", settings.settingsPath);
  loadSettingsFileDefaults(settings);

  if(arguments.find("--setting")) {
    string settingValue;
    while(arguments.take("--setting", settingValue)) {
      auto kv = nall::split(settingValue, "=", 1L);
      if(kv.size() != 2 || !applySettingOverride(settings, kv[0], kv[1])) {
        error = {"invalid setting: ", settingValue};
        return false;
      }
    }
  }

  return true;
}

}
