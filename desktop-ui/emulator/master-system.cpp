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
    //device.digital("Reset", virtualPorts[0].pad.rt);
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
    device.digital("1",     virtualPorts[id].pad.south);
    device.digital("2",     virtualPorts[id].pad.east);
    port.append(device); }

  { InputDevice device{"Paddle"};
    device.analog ("L-Left",  virtualPorts[id].pad.lstick_left);
    device.analog ("L-Right", virtualPorts[id].pad.lstick_right);
    device.analog ("X-Axis",  virtualPorts[id].pad.lstick_left, virtualPorts[id].pad.lstick_right);
    device.digital("Button",  virtualPorts[id].pad.south);
    port.append(device); }

  { InputDevice device{"Sports Pad"};
    device.relative("X", virtualPorts[id].mouse.x);
    device.relative("Y", virtualPorts[id].mouse.y);
    device.digital ("1", virtualPorts[id].mouse.left);
    device.digital ("2", virtualPorts[id].mouse.right);
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

  auto device = "Gamepad";
  if(game->pak->attribute("paddle").boolean()) device = "Paddle";
  if(game->pak->attribute("sportspad").boolean()) device = "Sports Pad";

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    port->allocate(device);
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 2")) {
    port->allocate(device);
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
