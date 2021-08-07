struct PCEngine : Emulator {
  PCEngine();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
};

PCEngine::PCEngine() {
  manufacturer = "NEC";
  name = "PC Engine";

  { InputPort port{"Controller Port"};

  { InputDevice device{"Gamepad"};
    device.digital("Up",     virtualPorts[0].pad.up);
    device.digital("Down",   virtualPorts[0].pad.down);
    device.digital("Left",   virtualPorts[0].pad.left);
    device.digital("Right",  virtualPorts[0].pad.right);
    device.digital("II",     virtualPorts[0].pad.b1);
    device.digital("I",      virtualPorts[0].pad.b2);
    device.digital("Select", virtualPorts[0].pad.select);
    device.digital("Run",    virtualPorts[0].pad.start);
    port.append(device); }

    ports.append(port);
  }
}

  // Multi-tap support: Skeleton
  // /* The console itself just have one controller port, but with Multi-tap up to 5 players can play in compatible games.
  //    It's a popular peripheral and as such, a lot of games in PCE/TurboGrafx permits multiplayer up to 5 players.
  //    eg.: all Bomberman games, Dungeon Explorer II, King of Casino. */
  // for(auto id : range(4) + 1) {
  // InputPort port{string{"Multi-tap Port ", 1 + id}};

  // { InputDevice device{"Gamepad"};
  //   device.digital("Up",     virtualPorts[id].pad.up);
  //   device.digital("Down",   virtualPorts[id].pad.down);
  //   device.digital("Left",   virtualPorts[id].pad.left);
  //   device.digital("Right",  virtualPorts[id].pad.right);
  //   device.digital("II",     virtualPorts[id].pad.b1);
  //   device.digital("I",      virtualPorts[id].pad.b2);
  //   device.digital("Select", virtualPorts[id].pad.select);
  //   device.digital("Run",    virtualPorts[id].pad.start);
  //   port.append(device); }

auto PCEngine::load() -> bool {
  game = mia::Medium::create("PC Engine");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  system = mia::System::create("PC Engine");
  if(!system->load()) return false;

  auto region = Emulator::region();
  string name = region == "NTSC-J" ? "PC Engine" : "TurboGrafx 16";
  if(!ares::PCEngine::load(root, {"[NEC] ", name, " (", region, ")"})) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port")) {
    port->allocate("Gamepad");
    port->connect();
  }

  return true;
}

auto PCEngine::save() -> bool {
  root->save();
  system->save(game->location);
  game->save(game->location);
  return true;
}

auto PCEngine::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "PC Engine") return system->pak;
  if(node->name() == "PC Engine Card") return game->pak;
  return {};
}
