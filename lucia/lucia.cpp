#include "lucia.hpp"

namespace ruby {
  Video video;
  Audio audio;
  Input input;
}

auto locate(const string& name) -> string {
  string location = {Path::program(), name};
  if(inode::exists(location)) return location;

  location = {Path::userData(), "lucia/", name};
  if(inode::exists(location)) return location;

  directory::create({Path::userSettings(), "lucia/"});
  return {Path::userSettings(), "lucia/", name};
}

#include <nall/main.hpp>
auto nall::main(Arguments arguments) -> void {
  Application::setName("lucia");
  Application::setScreenSaver(false);

  for(auto argument : arguments) {
    if(argument == "--fullscreen") {
      program.startFullScreen = true;
    } else if(file::exists(argument)) {
      program.startGameLoad = argument;
    }
  }

  Emulator::construct();
  inputManager.create();
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
