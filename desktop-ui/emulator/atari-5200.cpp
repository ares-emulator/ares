struct Atari5200 : Emulator {
  Atari5200();
  auto load() -> LoadResult override;
  auto save() -> bool override;
  auto pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> override;
};

Atari5200::Atari5200() {
  manufacturer = "Atari";
  name = "Atari 5200";

  firmware.push_back({
    "BIOS",
    "NTSC-U Four-port",
    "06b250f18983d058c0f156ce7ee88ae48b6eaf11e6f10f21dccf6ac7ffb6a6af"
  });
}

auto Atari5200::load() -> LoadResult {
  game = mia::Medium::create("Atari 5200");
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;

  LoadResult result = game->load(location);
  if(result != successful) return result;

  system = mia::System::create("Atari 5200");
  if(system->load(firmware[0].location) != successful) {
    result.firmwareSystemName = "Atari 5200";
    result.firmwareType = firmware[0].type;
    result.firmwareRegion = firmware[0].region;
    result.result = noFirmware;
    return result;
  }

  if(!ares::Atari5200::load(root, "[Atari] Atari 5200 (NTSC)")) return otherError;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  return successful;
}

auto Atari5200::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto Atari5200::pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> {
  if(node->name() == "Atari 5200") return system->pak;
  if(node->name() == "Atari 5200 Cartridge") return game->pak;
  return {};
}
