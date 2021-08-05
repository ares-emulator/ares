struct MSX : Emulator {
  MSX();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
};

MSX::MSX() {
  manufacturer = "Microsoft";
  name = "MSX";

  for(auto id : range(2)) {
    InputPort port{string{"Controller Port ", 1 + id}};

  { InputDevice device{"Gamepad"};
    device.digital("Up",    virtualPorts[id].pad.up);
    device.digital("Down",  virtualPorts[id].pad.down);
    device.digital("Left",  virtualPorts[id].pad.left);
    device.digital("Right", virtualPorts[id].pad.right);
    device.digital("A",     virtualPorts[id].pad.a);
    device.digital("B",     virtualPorts[id].pad.b);
    port.append(device); }

    ports.append(port);
  }
}

auto MSX::load() -> bool {
  game = mia::Medium::create("MSX");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  system = mia::System::create("MSX");
  if(!system->load()) return false;

  auto region = Emulator::region();
  if(!ares::MSX::load(root, {"[Microsoft] MSX (", region, ")"})) return false;

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

auto MSX::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto MSX::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "MSX") return system->pak;
  if(node->name() == "MSX Cartridge") return game->pak;
  return {};
}
