struct MasterSystem : Emulator {
  MasterSystem();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;

  u32 regionID = 0;
};

MasterSystem::MasterSystem() {
  manufacturer = "Sega";
  name = "Master System";

  firmware.append({"BIOS", "US"});      //NTSC-U
  firmware.append({"BIOS", "Japan"});   //NTSC-J
  firmware.append({"BIOS", "Europe"});  //PAL

  { InputPort port{"Master System"};

  { InputDevice device{"Controls"};
    device.digital("Pause", virtualPorts[0].pad.start);
    device.digital("Reset", virtualPorts[0].pad.rt);
    port.append(device); }

    ports.append(port);
  }

  for(auto id : range(2)) {
    InputPort port{string{"Controller Port ", 1 + id}};

  { InputDevice device{"Gamepad"};
    device.digital("Up",    virtualPorts[id].pad.up);
    device.digital("Down",  virtualPorts[id].pad.down);
    device.digital("Left",  virtualPorts[id].pad.left);
    device.digital("Right", virtualPorts[id].pad.right);
    device.digital("1",     virtualPorts[id].pad.a);
    device.digital("2",     virtualPorts[id].pad.b);
    port.append(device); }

    ports.append(port);
  }
}

auto MasterSystem::load() -> bool {
  game = mia::Medium::create("Master System");
  game->load(Emulator::load(game, configuration.game));

  auto region = Emulator::region();
  //if statements below are ordered by lowest to highest priority
  if(region == "PAL"   ) regionID = 2;
  if(region == "NTSC-J") regionID = 1;
  if(region == "NTSC-U") regionID = 0;

  system = mia::System::create("Master System");
  if(!system->load(firmware[regionID].location)) return false;
  if(!game->pak && !system->pak->read("bios.rom")) return false;

  if(!ares::MasterSystem::load(root, {"[Sega] Master System (", region, ")"})) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    port->allocate("Gamepad");
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 2")) {
    port->allocate("Gamepad");
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Expansion Port")) {
    port->allocate("FM Sound Unit");
    port->connect();
  }

  return true;
}

auto MasterSystem::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto MasterSystem::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Master System") return system->pak;
  if(node->name() == "Master System Cartridge") return game->pak;
  return {};
}
