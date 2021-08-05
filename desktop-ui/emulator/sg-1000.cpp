struct SG1000 : Emulator {
  SG1000();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
};

SG1000::SG1000() {
  manufacturer = "Sega";
  name = "SG-1000";

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

auto SG1000::load() -> bool {
  game = mia::Medium::create("SG-1000");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  system = mia::System::create("SG-1000");
  if(!system->load()) return false;

  auto region = Emulator::region();
  if(!ares::SG1000::load(root, {"[Sega] SG-1000 (", region, ")"})) return false;

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

  return true;
}

auto SG1000::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto SG1000::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "SG-1000") return system->pak;
  if(node->name() == "SG-1000 Cartridge") return game->pak;
  return {};
}
