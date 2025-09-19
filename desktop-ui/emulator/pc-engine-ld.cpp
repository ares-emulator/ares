struct PCEngineLD : PCEngine {
  PCEngineLD();
  auto load() -> LoadResult override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;

  shared_pointer<mia::Pak> bios;
  u32 biosID = 0;
};

PCEngineLD::PCEngineLD() {
  manufacturer = "NEC";
  name = "PC Engine LD";

  firmware.push_back({"PAC-N10", "US",    "????????????????????????????????????????????????????????????????"});
  firmware.push_back({"PAC-N1",  "Japan", "459325690a458baebd77495c91e37c4dddfdd542ba13a821ce954e5bb245627f"});
  firmware.push_back({"PCE-LP1", "Japan", "3f43b3b577117d84002e99cb0baeb97b0d65b1d70b4adadc68817185c6a687f0"});

  allocatePorts();
}

auto PCEngineLD::load() -> LoadResult {
  game = mia::Medium::create("PC Engine LD");
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;

  // Attempt to load the bios based on the desired region
  auto region = Emulator::region();
  bios = mia::Medium::create("PC Engine");
  biosID = 0;
  bool foundBiosVersion = false;
  while (!foundBiosVersion && (biosID < firmware.size())) {
    result = bios->load(firmware[biosID].location);
    foundBiosVersion = (result == successful);
    if (!foundBiosVersion) ++biosID;
  }
  if (!foundBiosVersion) {
    biosID = (region == "NTSC-J") ? 1 : 0;
    result.firmwareSystemName = "PC Engine";
    result.firmwareType = firmware[biosID].type;
    result.firmwareRegion = firmware[biosID].region;
    result.result = noFirmware;
    return result;
  }

  system = mia::System::create("PC Engine");
  result = system->load();
  if(result != successful) return result;

  ares::PCEngine::option("Pixel Accuracy", settings.video.pixelAccuracy);

  auto name = "[Pioneer] LaserActive (NEC PAC)";
  if(!ares::PCEngine::load(root, {name, " (", region, ")"})) return otherError;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("PC Engine LD/Disc Tray")) {
    port->allocate();
    port->connect();
  }

  connectPorts();

  return successful;
}

auto PCEngineLD::save() -> bool {
  root->save();
  system->save(game->location);
  bios->save(game->location);
  game->save(game->location);
  return true;
}

auto PCEngineLD::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "PC Engine") return system->pak;
  if(node->name() == "PC Engine Card") return bios->pak;
  if(node->name() == "PC Engine CD Disc") return game->pak;
  if(node->name() == "Laserdisc") return game->pak;
  return {};
}
