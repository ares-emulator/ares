#include "../desktop-ui.hpp"
#include "emulators.cpp"

std::vector<shared_pointer<Emulator>> emulators;
shared_pointer<Emulator> emulator;

auto Emulator::enumeratePorts(string name) -> std::vector<InputPort>& {
  for(auto& emulator : emulators) {
    if(emulator->name == name && !emulator->ports.empty()) return emulator->ports;
  }
  static std::vector<InputPort> ports;
  if(ports.empty()) {
    for(auto id : range(5)) {
      InputPort port{string{"Controller Port ", 1 + id}};
      port.append(virtualPorts[id].pad);
      port.append(virtualPorts[id].mouse);
      ports.push_back(port);
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
  auto preferredRegions = nall::split_and_strip(settings.boot.prefer, ",");
  if(game && game->pak) {
    auto regionList = game->pak->attribute("region");
    auto regions = nall::split_and_strip(regionList, ",");
    if(!regions.empty()) {
      for(auto &prefer: preferredRegions) {
        if(std::ranges::find(regions, prefer) != regions.end()) return prefer; //NTSC-U, NTSC-J or PAL
      }

      //Handle generic "NTSC" region.
      //NOTE: we don't need to check PAL because the above check covered it
      if(std::ranges::find(regions, string{"NTSC"}) != regions.end()) return "NTSC";

      //If no preferred region was found, return the first region in the list
      //NOTE: required for 'unsual' regions like NTSC-DEV for 64DD
      return regions.front();
    }
  }

  return {};
}


auto Emulator::handleLoadResult(LoadResult result) -> void {
  string errorText;

  switch (result.result) {
    case successful:
      return;
    case noFileSelected:
      return;
    case invalidROM:
      errorText = { "There was an error trying to parse the selected ROM. \n",
                    "Your ROM may be corrupt or contain a bad dump. " };
      break;
    case couldNotParseManifest:
      errorText = { "An error occurred while parsing the database file. You \n",
                    "may need to reinstall ares. " };
      break;
    case databaseNotFound:
      errorText = { "The database file for the system was not found. \n",
                    "Make sure that you have installed or packaged ares correctly. \n",
                    "Missing database file: " };
      break;
    case noFirmware:
      errorText = { "Error: firmware is missing or invalid.\n",
                    result.firmwareSystemName, " - ", result.firmwareType, " (", result.firmwareRegion, ") is required to play this game.\n",
                    "Would you like to configure firmware settings now? " };
      break;
    case romNotFound:
      errorText = "The selected ROM file was not found or could not be opened. ";
      break;
    case romNotFoundInDatabase:
      errorText = { "The required manifest for this ROM was not found in the database. \n",
                    "This title may not be currently supported by ares. " };
      break;
    case otherError:
      errorText = "An internal error occurred when initializing the emulator core. ";
      break;
  }
  
  if(result.info) {
    errorText = { errorText, result.info };
  }
  
  switch (result.result) {
    case noFirmware:
      if(MessageDialog().setText({
        errorText
      }).question() == "Yes") {
        settingsWindow.show("Firmware");
        firmwareSettings.select(emulator->name, result.firmwareType, result.firmwareRegion);
      }
      break;
    default:
      error(errorText);
  }
}

auto Emulator::load(const string& location) -> bool {
  Program::Guard guard;
  if(inode::exists(location)) locationQueue.push_back(location);
  
  LoadResult result = load();
  handleLoadResult(result);
  if(result != successful) {
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
  Program::Guard guard;
  string location;
  if(!locationQueue.empty()) {
    location = locationQueue.front();
    locationQueue.erase(locationQueue.begin());  //pull from the game queue if an entry is available
  } else if(!program.startGameLoad.empty()) {
    location = program.startGameLoad.front();
    program.startGameLoad.erase(program.startGameLoad.begin()); //pull from the command line if an entry is available
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
  Program::Guard guard;
  if(firmware.location.iendsWith(".zip")) {
    Decode::ZIP archive;
    if(archive.open(firmware.location) && !archive.file.empty()) {
      auto image = archive.extract(archive.file.front());
      return vfs::memory::open(image);
    }
  } else {
    auto image = file::read(firmware.location); 
    if(!image.empty()) {
      return vfs::memory::open(image);
    }
  }
  return {};
}

auto Emulator::unload() -> void {
  Program::Guard guard;
  save();
  root->unload();
  game = {};
  system = {};
  root.reset();
  locationQueue.clear();
}

auto Emulator::load(mia::Pak& node, string name) -> bool {
  Program::Guard guard;
  if(auto fp = node.pak->read(name)) {
    auto memory = file::read({node.location, name});
    if(!memory.empty()) {
      fp->read(memory);
      return true;
    }
  }
  return false;
}

auto Emulator::save(mia::Pak& node, string name) -> bool {
  Program::Guard guard;
  if(auto memory = node.pak->write(name)) {
    return file::write({node.location, name}, {memory->data(), memory->size()});
  }
  return false;
}

auto Emulator::refresh() -> void {
  Program::Guard guard;
  if(auto screen = root->scan<ares::Node::Video::Screen>("Screen")) {
    screen->refresh();
  }
}

auto Emulator::setBoolean(const string& name, bool value) -> bool {
  Program::Guard guard;
  if(auto node = root->scan<ares::Node::Setting::Boolean>(name)) {
    node->setValue(value);  //setValue() will not call modify() if value has not changed;
    node->modify(value);    //but that may prevent the initial setValue() from working
    return true;
  }
  return false;
}

auto Emulator::setOverscan(bool value) -> bool {
  Program::Guard guard;
  if(auto screen = root->scan<ares::Node::Video::Screen>("Screen")) {
    screen->setOverscan(value);
    return true;
  }
  return false;
}

auto Emulator::setColorBleed(bool value) -> bool {
  Program::Guard guard;
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

  lock_guard<recursive_mutex> programinputLock(program.inputMutex);
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
  lock_guard<recursive_mutex> programinputLock(program.inputMutex);
  for (auto& device : inputManager.devices) {
    if (!device->isKeyboard()) continue;

    auto keyboard = (shared_pointer<HID::Keyboard>)device;

    auto key = keyboard->buttons().find(name);
    if (!key) return false;

    return keyboard->buttons().input(*key).value();
  }

  return false;
}

