struct MegaDrive : Emulator {
  MegaDrive();
  auto load() -> LoadResult override;
  auto sixButtonIncompatible() const -> bool;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> std::shared_ptr<vfs::directory> override;

  std::shared_ptr<mia::Pak> disc;
  u32 regionID = 0;
};

MegaDrive::MegaDrive() {
  manufacturer = "Sega";
  name = "Mega Drive";

  for(auto id : range(2)) {
    InputPort port{string{"Controller Port ", 1 + id}};

  { InputDevice device{"Control Pad"};
    device.digital("Up",    virtualPorts[id].pad.up);
    device.digital("Down",  virtualPorts[id].pad.down);
    device.digital("Left",  virtualPorts[id].pad.left);
    device.digital("Right", virtualPorts[id].pad.right);
    device.digital("A",     virtualPorts[id].pad.west);
    device.digital("B",     virtualPorts[id].pad.south);
    device.digital("C",     virtualPorts[id].pad.east);
    device.digital("Start", virtualPorts[id].pad.start);
    port.append(device); }

  { InputDevice device{"Fighting Pad"};
    device.digital("Up",    virtualPorts[id].pad.up);
    device.digital("Down",  virtualPorts[id].pad.down);
    device.digital("Left",  virtualPorts[id].pad.left);
    device.digital("Right", virtualPorts[id].pad.right);
    device.digital("A",     virtualPorts[id].pad.west);
    device.digital("B",     virtualPorts[id].pad.south);
    device.digital("C",     virtualPorts[id].pad.east);
    device.digital("X",     virtualPorts[id].pad.l_bumper);
    device.digital("Y",     virtualPorts[id].pad.north);
    device.digital("Z",     virtualPorts[id].pad.r_bumper);
    device.digital("Mode",  virtualPorts[id].pad.select);
    device.digital("Start", virtualPorts[id].pad.start);
    port.append(device); }

  { InputDevice device{"Mega Mouse"};
    device.relative("X",      virtualPorts[id].mouse.x);
    device.relative("Y",      virtualPorts[id].mouse.y);
    device.digital ("Left",   virtualPorts[id].mouse.left);
    device.digital ("Right",  virtualPorts[id].mouse.right);
    device.digital ("Middle", virtualPorts[id].mouse.middle);
    device.digital ("Start",  virtualPorts[id].mouse.extra);
    port.append(device); }

    ports.push_back(port);
  }
}

auto MegaDrive::sixButtonIncompatible() const -> bool {
  if(!game || !game->pak) return false;

  auto serial = game->pak->attribute("serial");
  // Product codes documented at:
  // https://segaretro.org/Six_Button_Control_Pad_(Mega_Drive)
  static const std::vector<string> serialPrefixes = {
    "GM_T-081056",  // Arch Rivals - The Arcade Game (USA, Europe)
    "GM T-49116",   // Beast Wrestler (USA)
    "GM T-74023-",  // Double Dragon II - The Revenge (Japan)
    "GM T-50156",   // The Faery Tale Adventure (USA, Europe)
    "GM 00004016",  // Forgotten Worlds (World)
    "GM 00001122",  // Golden Axe II (World)
    "T-88056-50",   // International Rugby (Europe)
    "GM T-50016",   // John Madden Football (USA, Europe)
    "GM T-103026",  // King of the Monsters (USA)
    "GM G-4104 0",  // King of the Monsters (Japan)
    "GM MK-1086-",  // King of the Monsters (Europe)
    "GM MK-1210",   // Mario Lemieux Hockey (USA, Europe)
    "GM T-48036",   // Ms. Pac-Man (USA, Europe)
    "GM T-79016",   // Olympic Gold (Japan, USA, Europe)
    "GM T-113106",  // Second Samurai (Europe)
    "GM T-50216",   // Starflight (USA, Europe)
    "GM T-95026-",  // Sunset Riders (USA, Europe)
  };

  for(auto& prefix : serialPrefixes) {
    if(serial.beginsWith(prefix)) return true;
  }
  return false;
}

auto MegaDrive::load() -> LoadResult {
  game = mia::Medium::create("Mega Drive");
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;

  auto region = Emulator::region();
  //if statements below are ordered by lowest to highest priority
  if(region == "PAL"   ) regionID = 2;
  if(region == "NTSC-J") regionID = 1;
  if(region == "NTSC-U") regionID = 0;

  string name;
  if(game->pak->attribute("megacd").boolean()) {
    //use Mega CD firmware settings
    std::vector<Firmware> firmware;
    for(auto& emulator : emulators) {
      if(emulator->name == "Mega CD") firmware = emulator->firmware;
    }
    if(firmware.empty()) return otherError;  //should never occur
    name = "Mega CD";
    system = mia::System::create("Mega CD");
    result = system->load(firmware[regionID].location);
    if(result != successful) {
      result.firmwareSystemName = "Mega CD";
      result.firmwareType = firmware[regionID].type;
      result.firmwareRegion = firmware[regionID].region;
      result.result = noFirmware;
      return result;
    }

    disc = mia::Medium::create("Mega CD");
    if(disc->load(Emulator::load(disc, configuration.game)) != successful) disc.reset();
  } else {
    name = "Mega Drive";
    system = mia::System::create("Mega Drive");
    result = system->load();
    if(result != successful) return result;
  }

  ares::MegaDrive::option("TMSS", settings.megadrive.tmss);

  if(!ares::MegaDrive::load(root, {"[Sega] ", name, " (", region, ")"})) return otherError;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Mega CD/Disc Tray")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    port->allocate("Fighting Pad");
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 2")) {
    port->allocate("Fighting Pad");
    port->connect();
  }

  // A small set of games mis-handle the six-button protocol. Keep both ports
  // in three-button mode even if the normal default changes to Fighting Pad.
  if(sixButtonIncompatible()) {
    for(auto portName : {"Controller Port 1", "Controller Port 2"}) {
      if(auto port = root->find<ares::Node::Port>(portName)) {
        auto supported = port->supported();
        std::erase(supported, string{"Fighting Pad"});
        port->setSupported(supported);
        port->allocate("Control Pad");
        port->connect();
      }
    }
  }

  return successful;
}

auto MegaDrive::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  if(disc) disc->save(disc->location);
  return true;
}

auto MegaDrive::pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> {
  if(node->name() == "Mega Drive") return system->pak;
  if(node->name() == "Mega Drive Cartridge") return game->pak;
  if(node->name() == "Mega CD Disc" && disc) return disc->pak;
  return {};
}
