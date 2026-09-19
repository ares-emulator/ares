struct Gamate : Emulator {
  Gamate();
  auto load() -> LoadResult override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> std::shared_ptr<vfs::directory> override;
};

Gamate::Gamate() {
  manufacturer = "Bit Corporation";
  name = "Gamate";

  firmware.push_back({"BIOS", "World", "ea449dc607601f9a68d855ad6ab53800d2e99297"});

  { InputPort port{"Gamate"};

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

auto Gamate::load() -> LoadResult {
  game = mia::Medium::create("Gamate");
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;

  system = mia::System::create("Gamate");
  if(system->load(firmware[0].location) != successful) {
    result.firmwareSystemName = "Gamate";
    result.firmwareType = firmware[0].type;
    result.firmwareRegion = firmware[0].region;
    result.result = noFirmware;
    return result;
  }

  if(!ares::Gamate::load(root, "[Bit Corporation] Gamate")) return otherError;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  return successful;
}

auto Gamate::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto Gamate::pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> {
  if(node->name() == "Gamate") return system->pak;
  if(node->name() == "Gamate Cartridge") return game->pak;
  return {};
}
