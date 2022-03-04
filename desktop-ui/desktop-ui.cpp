#include "desktop-ui.hpp"

namespace ruby {
  Video video;
  Audio audio;
  Input input;
}

auto locate(const string& name) -> string {
  string location = {Path::program(), name};
  if(inode::exists(location)) return location;

  #if defined(PLATFORM_MACOS)
    location = {Path::program(), "../Resources/", name};
    if(inode::exists(location)) return location;
  #endif

  // On non-windows platforms, after exhausting other options,
  // default to userData, on Windows, default to program dir
  // most windows users desire portability.
  #if !defined(PLATFORM_WINDOWS)
    directory::create({Path::userData(), "ares/"});
    return {Path::userData(), "ares/", name};
  #else 
    return {Path::program(), name};
  #endif
}

#include <nall/main.hpp>
auto nall::main(Arguments arguments) -> void {
  Application::setName("ares");
  Application::setScreenSaver(false);

  mia::setHomeLocation([]() -> string {
    if(auto location = settings.paths.home) return location;
    return locate("Systems");
  });

  mia::setSaveLocation([]() -> string {
    return settings.paths.saves;
  });

  for(auto argument : arguments) {
    if(argument == "--fullscreen") {
      program.startFullScreen = true;
    } else if(file::exists(argument)) {
      program.startGameLoad = argument;
    }
  }

  inputManager.create();
  Emulator::construct();
  settings.load();
  Instances::presentation.construct();
  Instances::settingsWindow.construct();
  Instances::toolsWindow.construct();

  program.create();
  presentation.loadEmulators();
  Application::onMain({&Program::main, &program});
  Application::run();

  settings.save();

  Instances::presentation.destruct();
  Instances::settingsWindow.destruct();
  Instances::toolsWindow.destruct();
}
