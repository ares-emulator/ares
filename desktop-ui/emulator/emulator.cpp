#include "../desktop-ui.hpp"
#include "emulators.cpp"

vector<shared_pointer<Emulator>> emulators;
shared_pointer<Emulator> emulator;

auto Emulator::enumeratePorts(string name) -> vector<InputPort>& {
  for(auto& emulator : emulators) {
    if(emulator->name == name && emulator->ports) return emulator->ports;
  }
  static vector<InputPort> ports;
  if(!ports) {
    for(auto id : range(5)) {
      InputPort port{string{"Controller Port ", 1 + id}};
      port.append(virtualPorts[id].pad);
      port.append(virtualPorts[id].mouse);
      ports.append(port);
    }
  }
  return ports;
}

auto Emulator::location() -> string {
  return {Path::userData(), "ares/Saves/", name, "/"};
}

auto Emulator::locate(const string& location, const string& suffix, const string& path, maybe<string> system) -> string {
  if(!system) system = root->name();

  //game path
  if(!path) return {Location::notsuffix(location), suffix};

  //path override
  string pathname = {path, *system, "/"};
  directory::create(pathname);
  return {pathname, Location::prefix(location), suffix};
}

//handles region selection when games support multiple regions
auto Emulator::region() -> string {
  if(game && game->pak) {
    if(auto regions = game->pak->attribute("region").split(",").strip()) {
      if(settings.boot.prefer == "NTSC-U" && regions.find("NTSC-U")) return "NTSC-U";
      if(settings.boot.prefer == "NTSC-J" && regions.find("NTSC-J")) return "NTSC-J";
      if(settings.boot.prefer == "NTSC-U" && regions.find("NTSC"  )) return "NTSC";
      if(settings.boot.prefer == "NTSC-J" && regions.find("NTSC"  )) return "NTSC";
      if(settings.boot.prefer == "PAL"    && regions.find("PAL"   )) return "PAL";
      if(regions.first()) return regions.first();
    }
  }
  if(settings.boot.prefer == "NTSC-J") return "NTSC-J";
  if(settings.boot.prefer == "NTSC-U") return "NTSC-U";
  if(settings.boot.prefer == "PAL"   ) return "PAL";
  return {};
}

auto Emulator::handleLoadResult(LoadResult result) -> void {
  switch (result.result) {
    case successful:
      break;
    case noFileSelected:
      break;
    case couldNotParseManifest:
      error("An error was encountered while parsing the manifest. \n"
            "");
      break;
    case databaseNotFound:
      error("The database file for the system was not found. \n"
            "Make sure that you have installed or packaged ares correctly.");
      break;
    case noFirmware:
      if(MessageDialog().setText({
        "Error: firmware is missing or invalid.\n",
        result.firmwareSystemName, " - ", result.firmwareType, " (", result.firmwareRegion, ") is required to play this game.\n"
        "Would you like to configure firmware settings now?"
      }).question() == "Yes") {
        settingsWindow.show("Firmware");
        firmwareSettings.select(emulator->name, result.firmwareType, result.firmwareRegion);
      }
      break;
    case romNotFound:
      error("A required ROM file was not found.");
      break;
    case romNotFoundInDatabase:
      error("The selected ROM was not found in the database.");
      break;
    case otherError:
      error("Failed to load the selected system and ROM.");
      break;
  }
}

auto Emulator::load(const string& location) -> bool {
  if(inode::exists(location)) locationQueue.append(location);
  
  LoadResult result = load();
  handleLoadResult(result);
  if(result != LoadResult(successful)) {
    return false;
  }
  setBoolean("Color Emulation", settings.video.colorEmulation);
  setBoolean("Deep Black Boost", settings.video.deepBlackBoost);
  setBoolean("Interframe Blending", settings.video.interframeBlending);
  setOverscan(settings.video.overscan);
  setColorBleed(settings.video.colorBleed);

  latch = {};
  root->power();
  return true;
}

auto Emulator::load(shared_pointer<mia::Pak> pak, string& path) -> string {
  string location;
  if(locationQueue) {
    location = locationQueue.takeFirst();  //pull from the game queue if an entry is available
  } else if(program.startGameLoad) {
    location = program.startGameLoad.takeFirst(); //pull from the command line if an entry is available
  } else if(!program.noFilePrompt) {
    BrowserDialog dialog;
    dialog.setTitle({"Load ", pak->name(), " Game"});
    dialog.setPath(path ? path : Path::desktop());
    dialog.setAlignment(presentation);
    string filters{"*.zip:"};
    for(auto& extension : pak->extensions()) {
      filters.append("*.", extension, ":");
    }
    //support both uppercase and lowercase extensions
    filters.append(string{filters}.upcase());
    filters.trimRight(":", 1L);
    filters.prepend(pak->name(), "|");
    dialog.setFilters({filters, "All|*"});
    location = program.openFile(dialog);
  }

  if(location) {
    path = Location::dir(location);
    return location;
  }
  return {};
}

auto Emulator::loadFirmware(const Firmware& firmware) -> shared_pointer<vfs::file> {
  if(firmware.location.iendsWith(".zip")) {
    Decode::ZIP archive;
    if(archive.open(firmware.location) && archive.file) {
      auto image = archive.extract(archive.file.first());
      return vfs::memory::open(image);
    }
  } else if(auto image = file::read(firmware.location)) {
    return vfs::memory::open(image);
  }
  return {};
}

auto Emulator::unload() -> void {
  save();
  root->unload();
  game = {};
  system = {};
  root.reset();
  locationQueue.reset();
}

auto Emulator::load(mia::Pak& node, string name) -> bool {
  if(auto fp = node.pak->read(name)) {
    if(auto memory = file::read({node.location, name})) {
      fp->read(memory);
      return true;
    }
  }
  return false;
}

auto Emulator::save(mia::Pak& node, string name) -> bool {
  if(auto memory = node.pak->write(name)) {
    return file::write({node.location, name}, {memory->data(), memory->size()});
  }
  return false;
}

auto Emulator::refresh() -> void {
  if(auto screen = root->scan<ares::Node::Video::Screen>("Screen")) {
    screen->refresh();
  }
}

auto Emulator::setBoolean(const string& name, bool value) -> bool {
  if(auto node = root->scan<ares::Node::Setting::Boolean>(name)) {
    node->setValue(value);  //setValue() will not call modify() if value has not changed;
    node->modify(value);    //but that may prevent the initial setValue() from working
    return true;
  }
  return false;
}

auto Emulator::setOverscan(bool value) -> bool {
  if(auto screen = root->scan<ares::Node::Video::Screen>("Screen")) {
    screen->setOverscan(value);
    return true;
  }
  return false;
}

auto Emulator::setColorBleed(bool value) -> bool {
  if(auto screen = root->scan<ares::Node::Video::Screen>("Screen")) {
    screen->setColorBleed(screen->height() < 720 ? value : false);  //only apply to sub-HD content
    return true;
  }

  return false;
}

auto Emulator::error(const string& text) -> void {
  MessageDialog().setTitle("Error").setText(text).setAlignment(presentation).error();
}

auto Emulator::input(ares::Node::Input::Input input) -> void {
  //looking up inputs is very time-consuming; skip call if input was called too recently
  //note: allow rumble to be polled at full speed to prevent missed motor events
  if(!input->cast<ares::Node::Input::Rumble>()) {
    auto thisPoll = chrono::millisecond();
    if(thisPoll - input->lastPoll < 5) return;
    input->lastPoll = thisPoll;
  }

  auto device = ares::Node::parent(input);
  if(!device) return;

  auto port = ares::Node::parent(device);
  if(!port) return;
  
  for(auto& inputPort : ports) {
    if(inputPort.name != port->name()) continue;
    for(auto& inputDevice : inputPort.devices) {
      if(inputDevice.name != device->name()) continue;
      for(auto& inputNode : inputDevice.inputs) {
        if(inputNode.name != input->name()) continue;
        if(auto button = input->cast<ares::Node::Input::Button>()) {
          auto pressed = inputNode.mapping->pressed();
          return button->setValue(pressed);
        }
        if(auto axis = input->cast<ares::Node::Input::Axis>()) {
          auto value = inputNode.mapping->value();
          return axis->setValue(value);
        }
        if(auto rumble = input->cast<ares::Node::Input::Rumble>()) {
          if(auto target = dynamic_cast<InputRumble*>(inputNode.mapping)) {
            return target->rumble(rumble->strongValue(), rumble->weakValue());
          }
        }
      }
      for(auto& inputPair : inputDevice.pairs) {
        if(inputPair.name != input->name()) continue;
        if(auto axis = input->cast<ares::Node::Input::Axis>()) {
          auto value = inputPair.mappingHi->value() - inputPair.mappingLo->value();
          return axis->setValue(value);
        }
      }
    }
  }
}

auto Emulator::inputKeyboard(string name) -> bool {
  for (auto& device : inputManager.devices) {
    if (!device->isKeyboard()) continue;

    auto keyboard = (shared_pointer<HID::Keyboard>)device;

    auto key = keyboard->buttons().find(name);
    if (!key) return false;

    return keyboard->buttons().input(*key).value();
  }

  return false;
}

