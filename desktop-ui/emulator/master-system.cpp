struct MasterSystem : Emulator {
  MasterSystem();
  auto load() -> LoadResult override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;

  u32 regionID = 0;
};

MasterSystem::MasterSystem() {
  manufacturer = "Sega";
  name = "Master System";

  firmware.append({"BIOS", "US", "477617917a12a30f9f43844909dc2de6e6a617430f5c9a36306c86414a670d50"});      //NTSC-U
  firmware.append({"BIOS", "Japan", "67846e26764bd862f19179294347f7353a4166b62ac4198a5ec32933b7da486e"});   //NTSC-J
  firmware.append({"BIOS", "Europe", "477617917a12a30f9f43844909dc2de6e6a617430f5c9a36306c86414a670d50"});  //PAL

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

  { InputDevice device{"MD Control Pad"};
    device.digital("Up",    virtualPorts[id].pad.up);
    device.digital("Down",  virtualPorts[id].pad.down);
    device.digital("Left",  virtualPorts[id].pad.left);
    device.digital("Right", virtualPorts[id].pad.right);
    device.digital("A",     virtualPorts[id].pad.west);
    device.digital("B",     virtualPorts[id].pad.south);
    device.digital("C",     virtualPorts[id].pad.east);
    device.digital("Start", virtualPorts[id].pad.start);
    port.append(device); }

  { InputDevice device{"MD Fighting Pad"};
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

    ports.append(port);
  }
}

auto MasterSystem::load() -> LoadResult {
  game = mia::Medium::create("Master System");
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;

  auto region = Emulator::region();
  //if statements below are ordered by lowest to highest priority
  if(region == "PAL"   ) regionID = 2;
  if(region == "NTSC-J") regionID = 1;
  if(region == "NTSC-U") regionID = 0;

  system = mia::System::create("Master System");
  result = system->load(firmware[regionID].location);
  if(result != successful) return otherError;
  if(!game->pak && !system->pak->read("bios.rom")) return otherError;

  if(!ares::MasterSystem::load(root, {"[Sega] Master System (", region, ")"})) return otherError;

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

  return successful;
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
