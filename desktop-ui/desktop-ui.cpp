#include "desktop-ui.hpp"

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
  location = {Path::sharedData(), "ares/", name};
  if(inode::exists(location)) return location;

  // 4. The application bundle resource directory (macOS only)
#if defined(PLATFORM_MACOS)
  location = {Path::resources(), name};
  if(inode::exists(location)) return location;
#endif

  // If the file was not found in any of the above locations, we may be intending to create it
  // We must return a path to a user writable directory; on Windows, this is the executable directory
#if defined(PLATFORM_WINDOWS)
  return {Path::program(), name};
#endif

  // On other platforms, this is the "user data" directory
  directory::create({Path::userData(), "ares/"});
  return {Path::userData(), "ares/", name};
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

  inputManager.create();
  Emulator::construct();
  settings.load();

  if(arguments.find("--setting")) {
    string settingValue;
    while(arguments.take("--setting", settingValue)) {
      auto kv = settingValue.split("=", 1L);
      if(kv.size() == 2) {
        auto node = settings[kv[0]];
        if(node) {
          node.setValue(kv[1]);
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

  if(arguments.take("--help")) {
    print("Usage: ares [OPTIONS]... game(s)\n\n");
    print("Options:\n");
    print("  --help               Displays available options and exit\n");
#if defined(PLATFORM_WINDOWS)
    print("  --terminal           Create new terminal window\n");
#endif
    print("  --fullscreen         Start in full screen mode\n");
#if defined(PLATFORM_WINDOWS)	
    print("  --no/--Borderless    Disable/Enable Borderless Window");
#endif	
    print("  --system name        Specify the system name\n");
    print("  --shader name        Specify the name of the shader to use\n");
    print("  --setting name=value Specify a value for a setting\n");
    print("  --dump-all-settings  Show a list of all existing settings and exit\n");
    print("  --no-file-prompt     Do not prompt to load (optional) additional roms (eg: 64DD)\n");
    print("\n");
    print("Available Systems:\n");
    print("  ");
    for(auto& emulator : emulators) {
      print(emulator->name, ", ");
    }
    print("\n");
    return;
  }
  
#if defined(PLATFORM_WINDOWS)	
    if(arguments.take("--noBorderless")) {
        settings.general.borderless = false;
    }
    if(arguments.take("--Borderless")) {
        settings.general.borderless = true;
    }
#endif

  if(arguments.take("--dump-all-settings")) {
    function<void(const Markup::Node&, string)> dump;
    dump = [&](const Markup::Node& node, string prefix) -> void {
      for(const auto& setting : node) {
        print(prefix, setting.name(), "\n");
        dump(setting, string(prefix, setting.name(), "/"));
      }
    };
    dump(settings, "");
    return;
  }

  program.startGameLoad.reset();
  for(auto argument : arguments) {
    if(file::exists(argument)) program.startGameLoad.append(argument);
  }

  Instances::presentation.construct();
  Instances::settingsWindow.construct();
  Instances::toolsWindow.construct();
  Instances::gameBrowserWindow.construct();

  program.create();
  Application::onMain({&Program::main, &program});
  Application::run();

  settings.save();

  Instances::presentation.destruct();
  Instances::settingsWindow.destruct();
  Instances::toolsWindow.destruct();
  Instances::gameBrowserWindow.destruct();
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
