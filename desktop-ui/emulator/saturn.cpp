struct Saturn : Emulator {
  Saturn();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;

  u32 regionID = 0;
};

Saturn::Saturn() {
  manufacturer = "Sega";
  name = "Saturn";

  firmware.append({"BIOS", "US"});      //NTSC-U
  firmware.append({"BIOS", "Japan"});   //NTSC-J
  firmware.append({"BIOS", "Europe"});  //PAL

  for(auto id : range(2)) {
  InputPort port{string{"Controller Port ", 1 + id}};

  // Saturn controller skeleton
  // { InputDevice device{"Gamepad"};
  //   device.digital("Up",    virtualPorts[id].pad.up);
  //   device.digital("Down",  virtualPorts[id].pad.down);
  //   device.digital("Left",  virtualPorts[id].pad.left);
  //   device.digital("Right", virtualPorts[id].pad.right);
  //   device.digital("A",     virtualPorts[id].pad.b1);
  //   device.digital("B",     virtualPorts[id].pad.b2);
  //   device.digital("C",     virtualPorts[id].pad.b3);
  //   device.digital("X",     virtualPorts[id].pad.t1);
  //   device.digital("Y",     virtualPorts[id].pad.t2);
  //   device.digital("Z",     virtualPorts[id].pad.t3);
  //   device.digital("L",     virtualPorts[id].pad.l1);
  //   device.digital("R",     virtualPorts[id].pad.r1);
  //   device.digital("Start", virtualPorts[id].pad.start);
  //   port.append(device); }

  //   ports.append(port);
  }
}

auto Saturn::load() -> bool {
  game = mia::Medium::create("Saturn");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  auto region = Emulator::region();
  //if statements below are ordered by lowest to highest priority
  if(region == "PAL"   ) regionID = 2;
  if(region == "NTSC-J") regionID = 1;
  if(region == "NTSC-U") regionID = 0;

  system = mia::System::create("Saturn");
  if(!system->load(firmware[regionID].location)) return errorFirmware(firmware[regionID]), false;

  if(!ares::Saturn::load(root, {"[Sega] Saturn (", region, ")"})) return false;

  if(auto port = root->find<ares::Node::Port>("Saturn/Disc Tray")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    port->allocate("Gamepad");
    port->connect();
  }

  return true;
}

auto Saturn::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto Saturn::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Saturn") return system->pak;
  if(node->name() == "Saturn Disc") return game->pak;
  return {};
}
