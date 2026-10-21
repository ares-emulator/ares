struct GameBoyColor : Emulator {
  GameBoyColor();
  auto load() -> LoadResult override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> std::shared_ptr<vfs::directory> override;
};

GameBoyColor::GameBoyColor() {
  manufacturer = "Nintendo";
  name = "Game Boy Color";

  { InputPort port{"Game Boy Color"};

  { InputDevice device{"Controls"};
    device.digital("Up",      virtualPorts[0].pad.up);
    device.digital("Down",    virtualPorts[0].pad.down);
    device.digital("Left",    virtualPorts[0].pad.left);
    device.digital("Right",   virtualPorts[0].pad.right);
    device.digital("B",       virtualPorts[0].pad.south);
    device.digital("A",       virtualPorts[0].pad.east);
    device.digital("Select",  virtualPorts[0].pad.select);
    device.digital("Start",   virtualPorts[0].pad.start);
    device.rumble ("Rumble",  virtualPorts[0].pad.rumble);
    port.append(device); }

    ports.push_back(port);
  }
}

auto GameBoyColor::load() -> LoadResult {
  game = mia::Medium::create("Game Boy Color");
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;

  system = mia::System::create("Game Boy Color");
  result = system->load();
  if(result != successful) return result;

  if(!ares::GameBoy::load(root, "[Nintendo] Game Boy Color")) return otherError;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto fastBoot = root->find<ares::Node::Setting::Boolean>("Fast Boot")) {
    fastBoot->setValue(settings.boot.fast);
  }

  return successful;
}

auto GameBoyColor::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto GameBoyColor::pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> {
  if(node->name() == "Game Boy Color") return system->pak;
  if(node->name() == "Game Boy Color Cartridge") return game->pak;
  return {};
}
