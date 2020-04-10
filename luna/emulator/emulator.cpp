#include "../luna.hpp"
#include "platform.cpp"
#include "video.cpp"
#include "audio.cpp"
#include "input.cpp"
#include "states.cpp"
#include "status.cpp"
#include "utility.cpp"

Emulator emulator;

auto Emulator::create(shared_pointer<ares::Interface> instance, string location) -> void {
  interface = instance;

  system = {};
  system.name = Location::base(location).trimRight("/", 1L);
  system.data = location;
  system.templates = {Path::templates, interface->name(), "/"};

  string configuration = file::read({location, "settings.bml"});
  if(!configuration) {
    auto system = ares::Node::System::create();
    system->setName(interface->name());
    system->setAttribute("location", location);
    configuration = ares::Node::serialize(system);
  }

  //peripherals may have been renamed or deleted since last run; remove them from the configuration now
  auto document = BML::unserialize(configuration);
  for(auto node : document) validateConfiguration(node, document);
  configuration = BML::serialize(document);

  screens.reset();
  streams.reset();
  interface->load(root);
  root->copy(ares::Node::unserialize(configuration));

  systemMenu.setText(system.name);
  toolsMenu.pauseEmulation.setChecked(false);
  program.setEmulatorMode();
  program.setTitle(system.name);
  program.setFocused();

  videoUpdate();
  audioUpdate();
  inputManager.bind(root);
  inputManager.poll();

  power(false);
}

auto Emulator::unload() -> void {
  if(!interface) return;

  power(false);
  if(system.log) system.log.close();

  if(auto location = root->attribute("location")) {
    file::write({location, "settings.bml"}, ares::Node::serialize(root));
  }

  inputManager.unbind();

  root = {};
  interface->unload();
  interface.reset();

  program.setTitle({"luna v", ares::Version});
  systemMenu.setText("System");
  setCaption();
}

auto Emulator::main() -> void {
  updateMessage();
  if(Application::modal()) return;  //modal loop calls usleep() internally

  inputManager.poll();
  hotkeys.poll();

  if(!interface) return (void)usleep(20 * 1000);

  if(!system.power
  || program.toolsMenu.pauseEmulation.checked()
  || (!program.viewport.focused() && settings.input.unfocused == "Pause")
  ) {
    usleep(20 * 1000);
  } else {
    interface->run();
    if(events.power) power(false);  //system powered itself off
  }
}

auto Emulator::quit() -> void {
  //make quitting feel more responsive
  program.setVisible(false);
  Application::processEvents();

  //stop processing callbacks and timers
  Application::quit();

  unload();

  interfaces.reset();

  videoInstance.reset();
  audioInstance.reset();
  inputInstance.reset();
}

auto Emulator::power(bool on) -> void {
  if(system.power == on) return;
  if(system.power = on) {
    program.setTitle(interface->game());
    videoUpdateColors();
    audioUpdateEffects();
    events = {};
    interface->power();
    //powering on the system latches static settings
    nodeManager.refreshSettings();
    if(settingEditor.visible()) settingEditor.refresh();
    program.viewport.setFocused();
  } else {
    program.setTitle(system.name);
    setCaption();
    videoInstance.clear();
    audioInstance.clear();
    inputInstance.release();
  }
  systemMenu.power.setChecked(on);
  toolsMenu.saveStateMenu.setEnabled(on);
  toolsMenu.loadStateMenu.setEnabled(on);
}

//used to prevent connecting the same (emulated) physical device to multiple ports simultaneously
auto Emulator::connected(string location) -> ares::Node::Port {
  for(auto& peripheral : root->find<ares::Node::Peripheral>()) {
    if(location == peripheral->attribute("location")) {
      if(auto parent = peripheral->parent()) return parent.acquire();
    }
  }
  return {};
}

auto Emulator::validateConfiguration(Markup::Node node, Markup::Node parent) -> void {
  for(auto attribute : node.find("attribute")) {
    if(attribute["name"].text() != "location") continue;
    auto location = attribute["value"].text();
    //if the peripheral is missing, remove it from the tree
    if(!directory::exists(location)) parent.remove(node);
  }
  for(auto branch : node) validateConfiguration(branch, node);
}
