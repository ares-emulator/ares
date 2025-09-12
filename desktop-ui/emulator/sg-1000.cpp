struct SG1000 : Emulator {
  SG1000();
  auto load() -> LoadResult override;
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
    device.digital("1",     virtualPorts[id].pad.south);
    device.digital("2",     virtualPorts[id].pad.east);
    port.append(device); }

    ports.push_back(port);
  }
}

auto SG1000::load() -> LoadResult {
  game = mia::Medium::create("SG-1000");
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;

  system = mia::System::create("SG-1000");
  result = system->load();
  if(result != successful) return result;

  auto region = Emulator::region();
  if(!ares::SG1000::load(root, {"[Sega] SG-1000 (", region, ")"})) return otherError;

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
