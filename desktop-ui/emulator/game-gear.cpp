struct GameGear : Emulator {
  GameGear();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
};

GameGear::GameGear() {
  manufacturer = "Sega";
  name = "Game Gear";

  firmware.append({"BIOS", "World"});

  { InputPort port{"Game Gear"};

  { InputDevice device{"Controls"};
    device.digital("Up",    virtualPorts[0].pad.up);
    device.digital("Down",  virtualPorts[0].pad.down);
    device.digital("Left",  virtualPorts[0].pad.left);
    device.digital("Right", virtualPorts[0].pad.right);
    device.digital("1",     virtualPorts[0].pad.south);
    device.digital("2",     virtualPorts[0].pad.east);
    device.digital("Start", virtualPorts[0].pad.start);
    port.append(device); }

    ports.append(port);
  }
}

auto GameGear::load() -> bool {
  game = mia::Medium::create("Game Gear");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  auto region = Emulator::region();

  system = mia::System::create("Game Gear");
  if(!system->load(firmware[0].location)) return false;

  if(!ares::MasterSystem::load(root, {"[Sega] Game Gear (", region, ")"})) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  return true;
}

auto GameGear::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto GameGear::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Game Gear") return system->pak;
  if(node->name() == "Game Gear Cartridge") return game->pak;
  return {};
}
