struct MegaDuck : Emulator {
  MegaDuck();
  auto load() -> LoadResult override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> std::shared_ptr<vfs::directory> override;
};

MegaDuck::MegaDuck() {
  manufacturer = "Welback";
  name = "Mega Duck";

  { InputPort port{"Mega Duck"};

  { InputDevice device{"Controls"};
    device.digital("Up",     virtualPorts[0].pad.up);
    device.digital("Down",   virtualPorts[0].pad.down);
    device.digital("Left",   virtualPorts[0].pad.left);
    device.digital("Right",  virtualPorts[0].pad.right);
    device.digital("B",      virtualPorts[0].pad.south);
    device.digital("A",      virtualPorts[0].pad.east);
    device.digital("Select", virtualPorts[0].pad.select);
    device.digital("Start",  virtualPorts[0].pad.start);
    port.append(device); }

    ports.push_back(port);
  }
}

auto MegaDuck::load() -> LoadResult {
  game = mia::Medium::create("Mega Duck");
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;

  system = mia::System::create("Mega Duck");
  result = system->load();
  if(result != successful) return result;

  if(!ares::GameBoy::load(root, "[Welback] Mega Duck")) return otherError;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  return successful;
}

auto MegaDuck::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto MegaDuck::pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> {
  if(node->name() == "Mega Duck") return system->pak;
  if(node->name() == "Mega Duck Cartridge") return game->pak;
  return {};
}
