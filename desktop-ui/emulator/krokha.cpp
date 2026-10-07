struct Krokha: Emulator {
  Krokha();
  auto load() -> LoadResult override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> std::shared_ptr<vfs::directory> override;
};

Krokha::Krokha() {
  manufacturer = "SKB Kontur";
  name = "Krokha";

  { InputPort port{"Krokha"};

  { InputDevice device{"Controls"};
    device.digital("Up",    virtualPorts[0].pad.up);
    device.digital("Down",  virtualPorts[0].pad.down);
    device.digital("Left",  virtualPorts[0].pad.left);
    device.digital("Right", virtualPorts[0].pad.right);
    device.digital("Fire",  virtualPorts[0].pad.south);
    port.append(device); }

    ports.push_back(port);
  }
}

auto Krokha::load() -> LoadResult {
  game = mia::Medium::create("Krokha");
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;

  system = mia::System::create("Krokha");
  result = system->load();
  if(result != successful) return result;

  if(!ares::Krokha::load(root, "[SKB Kontur] Krokha")) return otherError;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  return successful;
}

auto Krokha::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto Krokha::pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> {
  if(node->name() == "Krokha") return system->pak;
  if(node->name() == "Krokha Cartridge") return game->pak;
  return {};
}
