struct Atari2600 : Emulator {
  Atari2600();
  auto load() -> LoadResult override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> std::shared_ptr<vfs::directory> override;
};

Atari2600::Atari2600() {
  manufacturer = "Atari";
  name = "Atari 2600";

  { InputPort port{"Atari 2600"};

  { InputDevice device{"Controls"};
    device.digital("Reset",            virtualPorts[0].pad.start);
    device.digital("Select",           virtualPorts[0].pad.select);
    device.digital("Left Difficulty",  virtualPorts[0].pad.l_bumper);
    device.digital("Right Difficulty", virtualPorts[0].pad.r_bumper);
    device.digital("TV Type",          virtualPorts[0].pad.north);
    port.append(device); }

    ports.push_back(port);
  }

  for(auto id : range(2)) {
    InputPort port{string{"Controller Port ", 1 + id}};

  { InputDevice device{"Gamepad"};
    device.digital("Up",         virtualPorts[id].pad.up);
    device.digital("Down",       virtualPorts[id].pad.down);
    device.digital("Left",       virtualPorts[id].pad.left);
    device.digital("Right",      virtualPorts[id].pad.right);
    device.digital("Fire",       virtualPorts[id].pad.south);
    port.append(device); }

  { InputDevice device{"Paddles"};
    device.analog ("Paddle 1 Left",  virtualPorts[id].pad.lstick_left);
    device.analog ("Paddle 1 Right", virtualPorts[id].pad.lstick_right);
    device.analog ("Paddle 2 Left",  virtualPorts[id].pad.rstick_left);
    device.analog ("Paddle 2 Right", virtualPorts[id].pad.rstick_right);
    device.digital("Paddle 1 Fire",  virtualPorts[id].pad.south);
    device.digital("Paddle 2 Fire",  virtualPorts[id].pad.east);
    device.analog ("Paddle 1",       virtualPorts[id].pad.lstick_left, virtualPorts[id].pad.lstick_right);
    device.analog ("Paddle 2",       virtualPorts[id].pad.rstick_left, virtualPorts[id].pad.rstick_right);
    port.append(device); }

  { InputDevice device{"Driving"};
    device.analog ("Wheel Left",  virtualPorts[id].pad.lstick_left);
    device.analog ("Wheel Right", virtualPorts[id].pad.lstick_right);
    device.digital("Fire",        virtualPorts[id].pad.south);
    device.analog ("Wheel",       virtualPorts[id].pad.lstick_left, virtualPorts[id].pad.lstick_right);
    port.append(device); }

  { InputDevice device{"Keyboard"};
    device.digital("1", virtualPorts[id].pad.one);
    device.digital("2", virtualPorts[id].pad.two);
    device.digital("3", virtualPorts[id].pad.three);
    device.digital("4", virtualPorts[id].pad.four);
    device.digital("5", virtualPorts[id].pad.five);
    device.digital("6", virtualPorts[id].pad.six);
    device.digital("7", virtualPorts[id].pad.seven);
    device.digital("8", virtualPorts[id].pad.eight);
    device.digital("9", virtualPorts[id].pad.nine);
    device.digital("*", virtualPorts[id].pad.star);
    device.digital("0", virtualPorts[id].pad.zero);
    device.digital("#", virtualPorts[id].pad.pound);
    port.append(device); }

    ports.push_back(port);
  }
}

auto Atari2600::load() -> LoadResult {
  game = mia::Medium::create("Atari 2600");
  string location = Emulator::load(game, configuration.game);
  if(!location) return couldNotParseManifest;
  LoadResult result = game->load(location);
  if(result != successful) return result;

  system = mia::System::create("Atari 2600");
  result = system->load();
  if(result != successful) return result;

  auto region = Emulator::region();
  if(!ares::Atari2600::load(root, {"[Atari] Atari 2600 (", region, ")"})) return otherError;

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

  return successful;
}

auto Atari2600::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto Atari2600::pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> {
  if(node->name() == "Atari 2600") return system->pak;
  if(node->name() == "Atari 2600 Cartridge") return game->pak;
  return {};
}
