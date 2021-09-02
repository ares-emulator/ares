struct PCEngineCD : Emulator {
  PCEngineCD();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;

  shared_pointer<mia::Pak> bios;
  u32 regionID = 0;
};

PCEngineCD::PCEngineCD() {
  manufacturer = "NEC";
  name = "PC Engine CD";

  firmware.append({"BIOS", "US"});     //NTSC-U
  firmware.append({"BIOS", "Japan"});  //NTSC-J

  for(auto id : range(5)) {
   InputPort port{string{"Controller Port ", 1 + id}};

  { InputDevice device{"Gamepad"};
    device.digital("Up",     virtualPorts[id].pad.up);
    device.digital("Down",   virtualPorts[id].pad.down);
    device.digital("Left",   virtualPorts[id].pad.left);
    device.digital("Right",  virtualPorts[id].pad.right);
    device.digital("II",     virtualPorts[id].pad.a);
    device.digital("I",      virtualPorts[id].pad.b);
    device.digital("Select", virtualPorts[id].pad.select);
    device.digital("Run",    virtualPorts[id].pad.start);
    port.append(device); }

    ports.append(port);
  }

  portBlacklist = {"Controller Port"};
  inputBlacklist = {"Multitap"};
}

auto PCEngineCD::load() -> bool {
  game = mia::Medium::create("PC Engine CD");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  auto region = Emulator::region();
  //if statements below are ordered by lowest to highest priority
  if(region == "NTSC-J") regionID = 1;
  if(region == "NTSC-U") regionID = 0;

  bios = mia::Medium::create("PC Engine");
  if(!bios->load(firmware[regionID].location)) return errorFirmware(firmware[regionID]), false;

  system = mia::System::create("PC Engine");
  if(!system->load()) return false;

  auto name = region == "NTSC-J" ? "PC Engine" : "TurboGrafx 16";
  if(!ares::PCEngine::load(root, {"[NEC] ", name, " (", region, ")"})) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("PC Engine CD/Disc Tray")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port")) {
    port->allocate("Multitap");
    port->connect();
  }

  for(auto id : range(5)) {
    if(auto port = root->scan<ares::Node::Port>(string{"Controller Port ", 1 + id})) {
      port->allocate("Gamepad");
      port->connect();
    }
  }

  return true;
}

auto PCEngineCD::save() -> bool {
  root->save();
  system->save(game->location);
  bios->save(game->location);
  game->save(game->location);
  return true;
}

auto PCEngineCD::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "PC Engine") return system->pak;
  if(node->name() == "PC Engine Card") return bios->pak;
  if(node->name() == "PC Engine CD Disc") return game->pak;
  return {};
}
