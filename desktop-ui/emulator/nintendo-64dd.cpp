//not functional yet

struct Nintendo64DD : Emulator {
  Nintendo64DD();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;

  shared_pointer<mia::Pak> bios;
};

Nintendo64DD::Nintendo64DD() {
  manufacturer = "Nintendo";
  name = "Nintendo 64DD";

  firmware.append({"BIOS", "Japan"});

  for(auto id : range(4)) {
    InputPort port{string{"Controller Port ", 1 + id}};

  { InputDevice device{"Gamepad"};
    device.analog ("L-Up",    virtualPorts[id].pad.lup);
    device.analog ("L-Down",  virtualPorts[id].pad.ldown);
    device.analog ("L-Left",  virtualPorts[id].pad.lleft);
    device.analog ("L-Right", virtualPorts[id].pad.lright);
    device.digital("Up",      virtualPorts[id].pad.up);
    device.digital("Down",    virtualPorts[id].pad.down);
    device.digital("Left",    virtualPorts[id].pad.left);
    device.digital("Right",   virtualPorts[id].pad.right);
    device.digital("B",       virtualPorts[id].pad.a);
    device.digital("A",       virtualPorts[id].pad.b);
    device.digital("C-Up",    virtualPorts[id].pad.rup);
    device.digital("C-Down",  virtualPorts[id].pad.rdown);
    device.digital("C-Left",  virtualPorts[id].pad.rleft);
    device.digital("C-Right", virtualPorts[id].pad.rright);
    device.digital("L",       virtualPorts[id].pad.l1);
    device.digital("R",       virtualPorts[id].pad.r1);
    device.digital("Z",       virtualPorts[id].pad.z);
    device.digital("Start",   virtualPorts[id].pad.start);
    device.rumble ("Rumble",  virtualPorts[id].pad.rumble);
    device.analog ("X-Axis",  virtualPorts[id].pad.lleft, virtualPorts[id].pad.lright);
    device.analog ("Y-Axis",  virtualPorts[id].pad.lup,   virtualPorts[id].pad.ldown);
    port.append(device); }

    ports.append(port);
  }
}

auto Nintendo64DD::load() -> bool {
  game = mia::Medium::create("Nintendo 64DD");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  bios = mia::Medium::create("Nintendo 64");
  if(!bios->load(firmware[0].location)) return errorFirmware(firmware[0]), false;

  system = mia::System::create("Nintendo 64");
  if(!system->load()) return false;

  ares::Nintendo64::option("Quality", settings.video.quality);
  ares::Nintendo64::option("Supersampling", settings.video.supersampling);

  auto region = Emulator::region();
  if(!ares::Nintendo64::load(root, {"[Nintendo] Nintendo 64 (", region, ")"})) return false;

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
  
  if(auto port = root->find<ares::Node::Port>("Controller Port 3")) {
    port->allocate("Gamepad");
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 4")) {
    port->allocate("Gamepad");
    port->connect();
  }
  return true;
}

auto Nintendo64DD::save() -> bool {
  root->save();
  system->save(system->location);
  bios->save(bios->location);
  game->save(game->location);
  return true;
}

auto Nintendo64DD::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Nintendo 64") return system->pak;
  if(node->name() == "Nintendo 64 Cartridge") return bios->pak;
  if(node->name() == "Nintendo 64DD Disk") return game->pak;
  return {};
}
