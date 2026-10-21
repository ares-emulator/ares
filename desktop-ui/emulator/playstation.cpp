struct PlayStation : Emulator {
  PlayStation();
  auto load() -> LoadResult override;
  auto load(Menu) -> void override;
  auto loadExpansion() -> LoadResult;
  auto saveExpansion() -> bool;
  auto unload() -> void override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> std::shared_ptr<vfs::directory> override;

  std::shared_ptr<mia::Pak> memoryCard;
  u32 regionID = 0;
  sTimer discTrayTimer;
  string expansionSavePath, expansionFirmwareHash;
};

PlayStation::PlayStation() {
  manufacturer = "Sony";
  name = "PlayStation";

  firmware.push_back({"BIOS", "US", "11052b6499e466bbf0a709b1f9cb6834a9418e66680387912451e971cf8a1fef"});      //NTSC-U
  firmware.push_back({"BIOS", "Japan", "9c0421858e217805f4abe18698afea8d5aa36ff0727eb8484944e00eb5e7eadb"});   //NTSC-J
  firmware.push_back({"BIOS", "Europe", "1faaa18fa820a0225e488d9f086296b8e6c46df739666093987ff7d8fd352c09"});  //PAL

  firmware.push_back({"Datel (128KB)", "Any"});
  firmware.push_back({"Datel (256KB)", "Any"});
  firmware.push_back({"Xplorer", "Any"});
  firmware.push_back({"Xplorer FX (Atmel)", "Any"});
  firmware.push_back({"Xplorer FX (Winbond)", "Any"});

  for(auto id : range(2)) {
    InputPort port{string{"Controller Port ", 1 + id}};

  { InputDevice device{"Digital Gamepad"};
    device.digital("Up",       virtualPorts[id].pad.up);
    device.digital("Down",     virtualPorts[id].pad.down);
    device.digital("Left",     virtualPorts[id].pad.left);
    device.digital("Right",    virtualPorts[id].pad.right);
    device.digital("Cross",    virtualPorts[id].pad.south);
    device.digital("Circle",   virtualPorts[id].pad.east);
    device.digital("Square",   virtualPorts[id].pad.west);
    device.digital("Triangle", virtualPorts[id].pad.north);
    device.digital("L1",       virtualPorts[id].pad.l_bumper);
    device.digital("L2",       virtualPorts[id].pad.l_trigger);
    device.digital("R1",       virtualPorts[id].pad.r_bumper);
    device.digital("R2",       virtualPorts[id].pad.r_trigger);
    device.digital("Select",   virtualPorts[id].pad.select);
    device.digital("Start",    virtualPorts[id].pad.start);
    port.append(device); }

  { InputDevice device{"DualShock"};
    device.analog ("L-Up",     virtualPorts[id].pad.lstick_up);
    device.analog ("L-Down",   virtualPorts[id].pad.lstick_down);
    device.analog ("L-Left",   virtualPorts[id].pad.lstick_left);
    device.analog ("L-Right",  virtualPorts[id].pad.lstick_right);
    device.analog ("R-Up",     virtualPorts[id].pad.rstick_up);
    device.analog ("R-Down",   virtualPorts[id].pad.rstick_down);
    device.analog ("R-Left",   virtualPorts[id].pad.rstick_left);
    device.analog ("R-Right",  virtualPorts[id].pad.rstick_right);
    device.digital("Up",       virtualPorts[id].pad.up);
    device.digital("Down",     virtualPorts[id].pad.down);
    device.digital("Left",     virtualPorts[id].pad.left);
    device.digital("Right",    virtualPorts[id].pad.right);
    device.digital("Cross",    virtualPorts[id].pad.south);
    device.digital("Circle",   virtualPorts[id].pad.east);
    device.digital("Square",   virtualPorts[id].pad.west);
    device.digital("Triangle", virtualPorts[id].pad.north);
    device.digital("L1",       virtualPorts[id].pad.l_bumper);
    device.digital("L2",       virtualPorts[id].pad.l_trigger);
    device.digital("L3",       virtualPorts[id].pad.lstick_click);
    device.digital("R1",       virtualPorts[id].pad.r_bumper);
    device.digital("R2",       virtualPorts[id].pad.r_trigger);
    device.digital("R3",       virtualPorts[id].pad.rstick_click);
    device.digital("Select",   virtualPorts[id].pad.select);
    device.digital("Start",    virtualPorts[id].pad.start);
    device.analog("L-Stick X", virtualPorts[id].pad.lstick_left, virtualPorts[id].pad.lstick_right);
    device.analog("L-Stick Y", virtualPorts[id].pad.lstick_up,   virtualPorts[id].pad.lstick_down);
    device.analog("R-Stick X", virtualPorts[id].pad.rstick_left, virtualPorts[id].pad.rstick_right);
    device.analog("R-Stick Y", virtualPorts[id].pad.rstick_up,   virtualPorts[id].pad.rstick_down);
    device.rumble("Rumble",    virtualPorts[0].pad.rumble);
    port.append(device); }

    ports.push_back(port);
  }
}

auto PlayStation::load() -> LoadResult {
  game = mia::Medium::create("PlayStation");
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;

  auto region = Emulator::region();
  //if statements below are ordered by lowest to highest priority
  if(region == "PAL"   ) regionID = 2;
  if(region == "NTSC-J") regionID = 1;
  if(region == "NTSC-U") regionID = 0;

  system = mia::System::create("PlayStation");
  result = system->load(firmware[regionID].location);
  if(result != successful) {
    result.firmwareSystemName = "PlayStation";
    result.firmwareType = firmware[regionID].type;
    result.firmwareRegion = firmware[regionID].region;
    result.result = noFirmware;
    return result;
  }

  result = loadExpansion();
  if(result != successful) return result;

  ares::PlayStation::option("Homebrew Mode", settings.developer.homebrewMode);
  ares::PlayStation::option("Recompiler", !settings.developer.forceInterpreter);
  ares::PlayStation::option("Pixel Accuracy", settings.video.pixelAccuracy);

  if(!ares::PlayStation::load(root, {"[Sony] PlayStation (", region, ")"})) return otherError;

  if(auto fastBoot = root->find<ares::Node::Setting::Boolean>("Fast Boot")) {
    fastBoot->setValue(settings.boot.fast);
  }

  if(auto port = root->find<ares::Node::Port>("PlayStation/Disc Tray")) {
    port->allocate();
    port->connect();
  }

  if(settings.playstation.expansionDevice != "None") {
    if(auto port = root->find<ares::Node::Port>("Parallel Port")) {
      port->allocate(settings.playstation.expansionDevice);
      port->connect();
    }
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    port->allocate("Digital Gamepad");
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Memory Card Port 1")) {
    memoryCard = mia::Pak::create("PlayStation");
    memoryCard->pak->append("save.card", 128_KiB);
    memoryCard->load("save.card", ".card", game->location);
    port->allocate("Memory Card");
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 2")) {
    port->allocate("Digital Gamepad");
    port->connect();
  }

  discTrayTimer = Timer{};

  return successful;
}

auto PlayStation::load(Menu menu) -> void {
  string switchPath = {"Parallel Port/", settings.playstation.expansionDevice, "/Cartridge Switch"};
  if(auto enabled = root->find<ares::Node::Setting::Boolean>(switchPath)) {
    MenuCheckItem item{&menu};
    item.setText("Cartridge Switch").setChecked(enabled->value());
    item.onToggle([enabled, item]() mutable {
      Program::Guard guard;
      enabled->setValue(item.checked());
    });
  }

  MenuItem changeDisc{&menu};
  changeDisc.setIcon(Icon::Device::Optical);
  changeDisc.setText("Change Disc").onActivate([&] {
    Program::Guard guard;
    save();
    auto tray = root->find<ares::Node::Port>("PlayStation/Disc Tray");
    tray->disconnect();

    if(game->load(Emulator::load(game, configuration.game)) != successful) {
      return;
    }

    //give the emulator core a few seconds to notice an empty drive state before reconnecting
    discTrayTimer->onActivate([&] {
      Program::Guard guard;
      discTrayTimer->setEnabled(false);
      auto tray = root->find<ares::Node::Port>("PlayStation/Disc Tray");
      tray->allocate();
      tray->connect();
    }).setInterval(3000).setEnabled();
  });
}

auto PlayStation::unload() -> void {
  Emulator::unload();
  discTrayTimer.reset();
}

auto PlayStation::save() -> bool {
  root->save();
  system->save(game->location);
  game->save(game->location);
  if(!saveExpansion()) return false;
  if(memoryCard) memoryCard->save("save.card", ".card", game->location);
  return true;
}

auto PlayStation::pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> {
  if(node->name() == "PlayStation") return system->pak;
  if(node->name() == "PlayStation Disc") return game->pak;
  if(node->name() == "Memory Card") return memoryCard->pak;
  return {};
}

auto PlayStation::loadExpansion() -> LoadResult {
  expansionSavePath.reset();
  expansionFirmwareHash.reset();
  for(auto& entry : firmware) {
    if(entry.type != settings.playstation.expansionDevice) continue;
    u32 firmwareSize = entry.type == "Xplorer FX (Atmel)" || entry.type == "Xplorer FX (Winbond)" ? 512_KiB : entry.type == "Datel (128KB)" ? 128_KiB : 256_KiB;
    auto image = loadFirmware(entry);
    if(!image || !image->size() || image->size() > firmwareSize) {
      LoadResult result = noFirmware;
      result.firmwareSystemName = "PlayStation";
      result.firmwareType = entry.type;
      result.firmwareRegion = "Any";
      return result;
    }
    std::vector<u8> original(image->size());
    image->seek(0); image->read({original.data(),original.size()});
    expansionFirmwareHash = Hash::SHA256(original).digest();
    std::vector<u8> eeprom(firmwareSize,0xff);
    memory::copy(eeprom.data(),original.data(),original.size());
    string saveRoot = settings.paths.saves ? settings.paths.saves : string{mia::homeLocation(), "Saves/"};
    if(!saveRoot.endsWith("/") && !saveRoot.endsWith("\\")) saveRoot.append("/");
    expansionSavePath = {saveRoot, "PlayStation/", entry.type, ".eeprom"};
    string hashPath = {expansionSavePath, ".firmware"};
    auto metadata = BML::unserialize(string::read(hashPath));
    if(metadata["firmware"].string() == expansionFirmwareHash) {
      auto saved = file::read(expansionSavePath);
      if(saved.size() == eeprom.size()) eeprom = std::move(saved);
    }
    system->pak->append("expansion.eeprom",eeprom);
    return successful;
  }
  return successful;
}

auto PlayStation::saveExpansion() -> bool {
  if(!expansionSavePath) return true;
  auto eeprom = system->pak->read("expansion.eeprom");
  if(!eeprom) return false;
  string metadata = {"firmware: ", expansionFirmwareHash, "\n"};
  directory::create(Location::path(expansionSavePath));
  if(!file::write(expansionSavePath,{eeprom->data(),eeprom->size()})) return false;
  return file::write({expansionSavePath,".firmware"},metadata);
}
