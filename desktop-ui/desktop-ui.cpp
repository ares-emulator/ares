#include "desktop-ui.hpp"
#include <map>

namespace ruby {
  Video video;
  Audio audio;
  Input input;
}

auto locate(const string& name) -> string {
  // First check each path for the presence of the file we are looking for in the following order
  // allowing users to override the default resources if they wish to do so.

  // 1. The application directory
  string location = {Path::program(), name};
  if(inode::exists(location)) return location;

  // 2. The user data directory
  location = {Path::userData(), "ares/", name};
  if(inode::exists(location)) return location;

  // 3. The shared data directory
#if defined(PLATFORM_LINUX) || defined(PLATFORM_BSD)
  /// Unix-like systems have multiple notions of a 'shared data' directory. First, check for
  /// an install prefix, as would be used by package managers that do not use `/usr/share`.
  /// Secondly, look in `/usr/local/share` to cover software compiled by the user.
  /// Lastly, look in the 'global' shared data directory, `/usr/share`.
  location = {Path::prefixSharedData(), "ares/", name};
  if(inode::exists(location)) return location;
  
  location = {Path::localSharedData(), "ares/", name};
  if(inode::exists(location)) return location;
#endif

  location = {Path::sharedData(), "ares/", name};
  if(inode::exists(location)) return location;

  // 4. The application bundle resource directory (macOS only)
#if defined(PLATFORM_MACOS)
  location = {Path::resources(), name};
  if(inode::exists(location)) return location;
#endif

  // If the file was not found in any of the above locations, we may be intending to create it
#if defined(PLATFORM_WINDOWS)
  // We must return a path to a user writable directory; on Windows, this is the executable directory
  return {Path::program(), name};
#else
  // On other platforms, this is the "user data" directory
  directory::create({Path::userData(), "ares/"});
  return {Path::userData(), "ares/", name};
#endif

}

#include <nall/main.hpp>
auto nall::main(Arguments arguments) -> void {
  //force early allocation for better proximity to executable code
  ares::Memory::FixedAllocator::get();

#if defined(PLATFORM_WINDOWS)
  bool createTerminal = arguments.take("--terminal");
  terminal::redirectStdioToTerminal(createTerminal);
#endif

  Application::setName("ares");
  Application::setScreenSaver(false);

  mia::setHomeLocation([]() -> string {
    if(auto location = settings.paths.home) return location;
    return locate("Systems/");
  });

  mia::setSaveLocation([]() -> string {
    return settings.paths.saves;
  });

  if(arguments.take("--fullscreen")) {
    program.startFullScreen = true;
  } else if(arguments.take("--pseudofullscreen")) {
    program.startPseudoFullScreen = true;
  }

  if(arguments.take("--kiosk")) {
    program.kiosk = true;
    program.noFilePrompt = true;
  }

  if(string system; arguments.take("--system", system)) {
    program.startSystem = system;
  }

  if(string shader; arguments.take("--shader", shader)) {
    program.startShader = shader;
  }

  if(arguments.take("--no-file-prompt")) {
    program.noFilePrompt = true;
  }

  settings.filePath = locate("settings.bml");
  if(string settingsFile; arguments.take("--settings-file", settingsFile)) {
    settings.filePath = settingsFile;
  }

  if(string savestate; arguments.take("--save-state", savestate)) {
    if(savestate.length() == 1 && savestate[0] >= '1' && savestate[0] <= '9') {
      program.startSaveStateSlot = savestate;
    }
  }

  if(arguments.take("--dump-settings")) {
    auto settingsFile = BML::unserialize(string::read(settings.filePath), " ");
    std::function<void(const Markup::Node&, string)> dump;
    dump = [&](const Markup::Node& node, string prefix) -> void {
      for(const auto& setting : node) {
        string path = prefix ? string{prefix, "/", setting.name()} : setting.name();
        if(setting.size() == 0) {
          string value = string{setting.string()}.strip();
          //empty mappings like ";;" represent unset bindings and should be omitted
          string mappingProbe = string{value}.replace(";", "").strip();
          if(value && mappingProbe) print(path, "=", value, "\n");
        }
        dump(setting, path);
      }
    };
    dump(settingsFile, {});
    return;
  }

  inputManager.create();
  Emulator::construct();

  static const std::vector<std::pair<string, std::vector<string>>> systemAliases = {
    {"Arcade", {"arc"}},
    {"Atari 2600", {"a2600"}},
    {"ColecoVision", {"cv"}},
    {"Famicom", {"fc", "nes"}},
    {"Famicom Disk System", {"fds"}},
    {"Game Boy", {"dmg", "gb"}},
    {"Game Boy Advance", {"agb", "gba"}},
    {"Game Boy Color", {"cgb", "gbc"}},
    {"Game Gear", {"gg"}},
    {"LaserActive (NEC PAC)", {"lnec", "pacn1", "pacn10"}},
    {"LaserActive (SEGA PAC)", {"lsega", "pacs1", "pacs10"}},
    {"MSX", {"msx"}},
    {"MSX2", {"msx2"}},
    {"Master System", {"ms", "sms"}},
    {"Mega 32X", {"32x"}},
    {"Mega CD", {"mcd"}},
    {"Mega CD 32X", {"mcd32x"}},
    {"Mega Drive", {"gen", "genesis", "md", "sg"}},
    {"MyVision", {"mv"}},
    {"Neo Geo AES", {"aes", "ngaes"}},
    {"Neo Geo MVS", {"mvs", "ngmvs"}},
    {"Neo Geo Pocket", {"ngp"}},
    {"Neo Geo Pocket Color", {"ngpc"}},
    {"Nintendo 64", {"n64"}},
    {"Nintendo 64DD", {"n64dd"}},
    {"PC Engine", {"pce"}},
    {"PC Engine CD", {"pcecd"}},
    {"PlayStation", {"ps", "ps1", "psx"}},
    {"Pocket Challenge V2", {"pcv2"}},
    {"SC-3000", {"sc3000"}},
    {"SG-1000", {"sg1000"}},
    {"Saturn", {"sat", "ss"}},
    {"Super Famicom", {"sfc", "snes"}},
    {"SuperGrafx", {"sgx"}},
    {"SuperGrafx CD", {"sgxcd"}},
    {"WonderSwan", {"ws"}},
    {"WonderSwan Color", {"wsc"}},
    {"ZX Spectrum", {"zx"}},
    {"ZX Spectrum 128", {"zx128"}},
  };

  auto resolveSystemName = [&](const string& name) -> string {
    auto alias = string{name}.downcase();
    for(const auto& [system, aliases] : systemAliases) {
      if(std::ranges::find(aliases, alias) != aliases.end()) return system;
    }

    return name;
  };

  if(program.startSystem) {
    program.startSystem = resolveSystemName(program.startSystem);
  }

  settings.load();

  //normalize for the setting key. "LaserActive (NEC PAC)" => "LaserActiveNECPAC"
  auto settingKey = [](string text) -> string {
    return string{text}.replace(" ", "").replace("(", "").replace(")", "").replace("/", "").replace("\\", "");
  };

  std::vector<string> portSettings;
  std::vector<string> peripheralSettings;
  for(auto& emulator : emulators) {
    string core = settingKey(emulator->name);
    for(auto& port : emulator->ports) {
      if(port.devices.size() <= 1) continue;
      portSettings.push_back({"Ports/", core, "/", settingKey(port.name)});
    }

    if(core == "Nintendo64" || core == "Nintendo64DD") {
      for(auto& port : emulator->ports) {
        string portKey = settingKey(port.name);
        if(!portKey.beginsWith("ControllerPort")) continue;
        peripheralSettings.push_back({"Peripherals/", core, "/", portKey});
      }
    }
  }

  //for settings that don't persist in settings.bml because they are dynamically generated
  //based on the loaded core
  auto isDynamicSetting = [&](const string& path) -> bool {
    if(std::ranges::find(portSettings, path) != portSettings.end()) return true;
    if(std::ranges::find(peripheralSettings, path) != peripheralSettings.end()) return true;
    return false;
  };

  auto enumerateDumpableSettings = [&]() -> std::vector<string> {
    std::vector<string> dumpableSettings;
    std::function<void(const Markup::Node&, string)> dump;
    dump = [&](const Markup::Node& node, string prefix) -> void {
      for(const auto& setting : node) {
        string path = prefix ? string{prefix, "/", setting.name()} : setting.name();
        bool isLeaf = setting.size() == 0;
        if(isLeaf) {
          auto parts = nall::split(path, "/");
          if(!parts.empty() && parts[0] != "Ports" && parts[0] != "Peripherals") {
            dumpableSettings.push_back(path);
          }
        }
        dump(setting, path);
      }
    };
    dump(settings, {});
    for(auto& path : portSettings) dumpableSettings.push_back(path);
    for(auto& path : peripheralSettings) dumpableSettings.push_back(path);
    return dumpableSettings;
  };

  if(arguments.find("--setting")) {
    string settingValue;
    while(arguments.take("--setting", settingValue)) {
      auto kv = nall::split(settingValue, "=", 1L);
      if(kv.size() == 2) {
        auto node = settings[kv[0]];
        if(node) {
          node.setValue(kv[1]);
        } else if(isDynamicSetting(kv[0])) {
          settings(kv[0]).setValue(kv[1]);
        } else {
          print("Invalid setting: ", settingValue, "\n");
          return;
        }
      } else {
        print("Invalid setting: ", settingValue, "\n");
        return;
      }
    }
    settings.process(true);
  }

  if(program.noFilePrompt) settings.general.noFilePrompt = true;

  if(arguments.take("--help")) {
    print("\n Usage: ares [OPTIONS]... game(s)\n\n");
    print("Options:\n");
    print("  --help                Displays available options and exit\n");
    print("  --version             Displays the version string of the application\n");
#if defined(PLATFORM_WINDOWS)
    print("  --terminal            Create new terminal window\n");
#endif
    print("  --fullscreen          Start in full screen mode\n");
    print("  --pseudofullscreen    Start in psuedo full screen mode\n");
    print("  --kiosk               Start in minimal UI mode (implies --no-file-prompt)\n");
    print("  --system name         Specify the system name (supports aliases, eg: fc, n64 etc.)\n");
    print("  --shader name         Specify the name of the shader to use\n");
    print("  --setting name=value  Specify a value for a setting\n");
    print("  --dump-settings       Print the current settings and exit\n");
    print("  --list-settings       Print all configurable setting keys and exit\n");
    print("  --list-setting-values Print valid values for each setting key and exit\n");
    print("  --list-sys-aliases    Print valid system aliases and exit\n");
    print("  --no-file-prompt      Do not prompt to load (optional) additional roms (eg: 64DD)\n");
    print("  --settings-file path  Specify a settings file override (settings.bml)\n");
    print("  --save-state slot     Specify a save state slot to load (1-9)\n");
    print("\n");
    print("Available Systems:\n");
    print("  ");
    for(auto& emulator : emulators) {
      print(emulator->name, ", ");
    }
    print("\n\nares version ", ares::Version, "\n");
    return;
  }

  if(arguments.take("--version")) {
    print("\n", ares::Version, "\n");
    return;
  }

  if(arguments.take("--list-settings")) {
    for(const auto& path : enumerateDumpableSettings()) {
      print(path, "\n");
    }
    return;
  }

  if(arguments.take("--list-sys-aliases")) {
    for(const auto& [system, aliases] : systemAliases) {
      print(system, ": ");
      for(u32 index : range(aliases.size())) {
        if(index) print(", ");
        print(aliases[index]);
      }
      print("\n");
    }
    return;
  }

  if(arguments.take("--list-setting-values")) {
    auto joinChoices = [](const std::vector<string>& values) -> string {
      string output;
      for(size_t index = 0; index < values.size(); ++index) {
        if(index) output.append("|");
        output.append(values[index]);
      }
      return output;
    };

    std::map<string, std::vector<string>> choiceMap;
    auto appendChoice = [&](const string& path, const string& value) -> void {
      auto& choices = choiceMap[path];
      if(std::ranges::find(choices, value) == choices.end()) choices.push_back(value);
    };
    auto appendChoices = [&](const string& path, const std::vector<string>& values) -> void {
      for(const auto& value : values) appendChoice(path, value);
    };

    appendChoices("Video/Driver", ruby::video.hasDrivers());
    appendChoice("Video/Monitor", "Primary");
    if(settings.video.monitor && settings.video.monitor != "Primary") appendChoice("Video/Monitor", settings.video.monitor);
    appendChoices("Video/Format", ruby::video.hasFormats());
    appendChoices("Audio/Driver", ruby::audio.hasDrivers());
    appendChoices("Audio/Device", ruby::audio.hasDevices());
    for(auto frequency : ruby::audio.hasFrequencies()) appendChoice("Audio/Frequency", frequency);
    for(auto latency : ruby::audio.hasLatencies()) appendChoice("Audio/Latency", latency);
    appendChoices("Input/Driver", ruby::input.hasDrivers());
    appendChoices("Input/Defocus", {"Pause", "Block", "Allow"});
    appendChoices("Video/Quality", {"SD", "HD", "UHD"});
    appendChoices("Video/Output", {"Scale", "Integer", "Stretch"});
    appendChoices("Video/AspectCorrectionMode", {"None", "Standard", "Anamorphic"});
    appendChoices("Boot/Prefer", {
      "NTSC-U,NTSC-J,PAL",
      "NTSC-U,PAL,NTSC-J",
      "NTSC-J,NTSC-U,PAL",
      "NTSC-J,PAL,NTSC-U",
      "PAL,NTSC-U,NTSC-J",
      "PAL,NTSC-J,NTSC-U"
    });
    appendChoices("Nintendo64/ControllerPakBankString", {
      "32KiB (Default)",
      "128KiB (Datel 1Meg)",
      "512KiB (Datel 4Meg)",
      "1984KiB (Maximum)"
    });

    for(auto& emulator : emulators) {
      string core = settingKey(emulator->name);
      for(auto& port : emulator->ports) {
        if(port.devices.size() <= 1) continue;
        string path = {"Ports/", core, "/", settingKey(port.name)};
        for(auto& device : port.devices) appendChoice(path, device.name);
      }

      if(core == "Nintendo64" || core == "Nintendo64DD") {
        for(auto& port : emulator->ports) {
          string portKey = settingKey(port.name);
          if(!portKey.beginsWith("ControllerPort")) continue;
          string path = {"Peripherals/", core, "/", portKey};
          appendChoices(path, {"Rumble Pak", "Controller Pak", "Transfer Pak", "Bio Sensor", "None"});
        }
      }
    }

    for(const auto& path : enumerateDumpableSettings()) {
      std::vector<string> choices;
      if(choiceMap.contains(path)) {
        choices = choiceMap[path];
      } else {
        auto value = settings[path].string();
        if(value == "true" || value == "false") {
          choices = {"true", "false"};
        } else {
          choices = {"<value>"};
        }
      }
      print(path, "=", joinChoices(choices), "\n");
    }
    return;
  }

  program.startGameLoad.clear();
  std::vector<string> invalidKioskPaths;
  for(auto argument : arguments) {
    if(file::exists(argument) || directory::exists(argument)) {
      program.startGameLoad.push_back(argument);
    } else if(program.kiosk) {
      invalidKioskPaths.push_back(argument);
    }
  }

  if(program.kiosk) {
    if(!invalidKioskPaths.empty()) {
      program.error({"path does not exist: ", invalidKioskPaths.front()});
      return;
    }
    if(program.startGameLoad.empty()) {
      program.error("provide a valid game file or directory.");
      return;
    }
  }

  if(program.startSystem && !program.startGameLoad.empty()) {
    bool foundSystem = false;
    for(auto& emulator : emulators) {
      if(emulator->name == program.startSystem) {
        foundSystem = true;
        break;
      }
    }
    if(!foundSystem) {
      auto text = string{"Unrecognized argument for --system: ", program.startSystem, "\n"
                         "Use --help to list all valid systems supported by ares."};
      program.error(text);
      if(program.kiosk) return;
    }
  }

  Instances::presentation.construct();
  if(!program.kiosk) {
    Instances::settingsWindow.construct();
    program.settingsWindowConstructed = true;
    Instances::gameBrowserWindow.construct();
    program.gameBrowserWindowConstructed = true;
    Instances::toolsWindow.construct();
    program.toolsWindowConstructed = true;
  }

  program.create();
  Application::onMain(std::bind_front(&Program::main, &program));
  Application::run();

  settings.save();

  Instances::presentation.destruct();
  if(program.settingsWindowConstructed) Instances::settingsWindow.destruct();
  if(program.toolsWindowConstructed) Instances::toolsWindow.destruct();
  if(program.gameBrowserWindowConstructed) Instances::gameBrowserWindow.destruct();
}

#if defined(PLATFORM_WINDOWS) && defined(ARCHITECTURE_AMD64) && !defined(BUILD_LOCAL)

#include <nall/windows/windows.hpp>
#include <intrin.h>

//this code must run before C++ global initializers
//it works with any valid combination of GCC, Clang, or MSVC and MingW or MSVCRT
//ref: https://learn.microsoft.com/en-us/cpp/c-runtime-library/crt-initialization

auto preCppInitializer() -> int {
  int data[4] = {};
  __cpuid(data, 1);
  bool sse42 = data[2] & 1 << 20;
  if(!sse42) FatalAppExitA(0, "This build of ares requires a CPU that supports SSE4.2.");
  return 0;
}

extern "C" {
#if defined(_MSC_VER)
  #pragma comment(linker, "/include:preCppInitializerEntry")
  #pragma section(".CRT$XCT", read)
  __declspec(allocate(".CRT$XCT"))
#else
  __attribute__((section (".CRT$XCT"), used))
#endif
  decltype(&preCppInitializer) preCppInitializerEntry = preCppInitializer;
}

#endif
