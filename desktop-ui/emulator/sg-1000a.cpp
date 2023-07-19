struct SG1000A : Emulator {
  SG1000A();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto arcade() -> bool override { return true; }
};

SG1000A::SG1000A() {
  manufacturer = "Sega";
  name = "SG-1000 Arcade System";
  medium = "SG-1000A";

  { InputPort port{string{"SG-1000A"}};

  { InputDevice device{"Arcade Controls"};
    device.digital("Player 1 Up",       virtualPorts[0].pad.up);
    device.digital("Player 1 Down",     virtualPorts[0].pad.down);
    device.digital("Player 1 Left",     virtualPorts[0].pad.left);
    device.digital("Player 1 Right",    virtualPorts[0].pad.right);
    device.digital("Player 1 Button 1", virtualPorts[0].pad.south);
    device.digital("Player 1 Button 2", virtualPorts[0].pad.east);
    device.digital("Player 1 Start",    virtualPorts[0].pad.start);
    device.digital("Coin",              virtualPorts[0].pad.select);
    device.digital("Service",           virtualPorts[0].pad.north);
    device.digital("Player 2 Up",       virtualPorts[1].pad.up);
    device.digital("Player 2 Down",     virtualPorts[1].pad.down);
    device.digital("Player 2 Left",     virtualPorts[1].pad.left);
    device.digital("Player 2 Right",    virtualPorts[1].pad.right);
    device.digital("Player 2 Button 1", virtualPorts[1].pad.south);
    device.digital("Player 2 Button 2", virtualPorts[1].pad.east);
    device.digital("Player 2 Start",    virtualPorts[1].pad.start);
    port.append(device); }

    ports.append(port);
  }
}

auto SG1000A::load() -> bool {
  game = mia::Medium::create("SG-1000A");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  system = mia::System::create("SG-1000A");
  if(!system->load()) return false;

  auto region = Emulator::region();
  if(!ares::SG1000::load(root, {"[Sega] SG-1000A"})) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  return true;
}

auto SG1000A::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto SG1000A::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "SG-1000A") return system->pak;
  if(node->name() == "SG-1000A Cartridge") return game->pak;
  return {};
}
