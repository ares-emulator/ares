struct SuperGrafxCD : PCEngine {
  SuperGrafxCD();
  auto load() -> LoadResult override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> std::shared_ptr<vfs::directory> override;

  std::shared_ptr<mia::Pak> bios;
  u32 regionID = 0;
};

SuperGrafxCD::SuperGrafxCD() {
  manufacturer = "NEC";
  name = "SuperGrafx CD";

  firmware.push_back({"Arcade Card", "Japan", "e11527b3b96ce112a037138988ca72fd117a6b0779c2480d9e03eaebece3d9ce"});

  allocatePorts();
}

auto SuperGrafxCD::load() -> LoadResult {
  game = std::dynamic_pointer_cast<mia::Pak>(mia::Medium::create("PC Engine CD"));
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;

  bios = std::dynamic_pointer_cast<mia::Pak>(mia::Medium::create("PC Engine"));
  result = bios->load(firmware[0].location);
  if(result != successful) {
    result.firmwareSystemName = "SuperGrafx CD";
    result.firmwareType = firmware[0].type;
    result.firmwareRegion = firmware[0].region;
    result.result = noFirmware;
    return result;
  }

  system = mia::System::create("SuperGrafx");
  result = system->load();
  if(result != successful) return result;

  ares::PCEngine::option("Pixel Accuracy", settings.video.pixelAccuracy);

  if(!ares::PCEngine::load(root, "[NEC] SuperGrafx (NTSC-J)")) return otherError;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("PC Engine CD/Disc Tray")) {
    port->allocate();
    port->connect();
  }

  connectPorts();

  return successful;
}

auto SuperGrafxCD::save() -> bool {
  root->save();
  system->save(game->location);
  bios->save(game->location);
  game->save(game->location);
  return true;
}

auto SuperGrafxCD::pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> {
  if(node->name() == "SuperGrafx") return system->pak;
  if(node->name() == "SuperGrafx Card") return bios->pak;
  if(node->name() == "PC Engine CD Disc") return game->pak;
  return {};
}
