struct GameGear : Emulator {
  GameGear();
  auto load() -> LoadResult override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> std::shared_ptr<vfs::directory> override;
};

GameGear::GameGear() {
  manufacturer = "Sega";
  name = "Game Gear";

  firmware.push_back({"BIOS", "World", "8c8a21335038285cfa03dc076100c1f0bfadf3e4ff70796f11f3dfaaab60eee2"});

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

    ports.push_back(port);
  }
}

auto GameGear::load() -> LoadResult {
  game = mia::Medium::create("Game Gear");
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;

  auto region = Emulator::region();

  system = mia::System::create("Game Gear");
  result = system->load(firmware[0].location);
  if(result != successful) return otherError;

  if(!ares::MasterSystem::load(root, {"[Sega] Game Gear (", region, ")"})) return otherError;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  return successful;
}

auto GameGear::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto GameGear::pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> {
  if(node->name() == "Game Gear") return system->pak;
  if(node->name() == "Game Gear Cartridge") return game->pak;
  return {};
}
