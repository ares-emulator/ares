struct PCEngineLD : PCEngine {
  PCEngineLD();
  auto load() -> LoadResult override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;

  shared_pointer<mia::Pak> hucard;
  u32 internalBiosId = 0;
  maybe<u32> hucardBiosId;
};

PCEngineLD::PCEngineLD() {
  manufacturer = "Pioneer";
  name = "LaserActive (NEC PAC)";

  firmware.push_back({"PAC-N10", "US",    "0e87a3385a27b3a4cac51934819b7eefa5b3d690768d2495633838488cd0e2e4"});
  firmware.push_back({"PAC-N1",  "Japan", "459325690a458baebd77495c91e37c4dddfdd542ba13a821ce954e5bb245627f"});
  firmware.push_back({"PCE-LP1", "Japan", "3f43b3b577117d84002e99cb0baeb97b0d65b1d70b4adadc68817185c6a687f0"});

  firmware.push_back({"System-Card 1.0", "Japan", "afe9f27f91ac918348555b86298b4f984643eafa2773196f2c5441ea84f0c3bb"});
  firmware.push_back({"Games Express"  , "Japan", "4b86bb96a48a4ca8375fc0109631d0b1d64f255a03b01de70594d40788ba6c3d"});
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
  system = mia::System::create("PC Engine LD");
  internalBiosId = 0;
  bool foundBiosVersion = false;
  while (!foundBiosVersion && (internalBiosId < firmware.size())) {
    result = system->load(firmware[internalBiosId].location);
    foundBiosVersion = (result == successful);
    if (!foundBiosVersion) ++internalBiosId;
  }
  if (!foundBiosVersion) {
    internalBiosId = (region == "NTSC-J") ? 1 : 0;
    result.firmwareSystemName = "PC Engine";
    result.firmwareType = firmware[internalBiosId].type;
    result.firmwareRegion = firmware[internalBiosId].region;
    result.result = noFirmware;
    return result;
  }

  // Some games require a specific card to be inserted into the system
  if(auto requiredCard = game->pak->attribute("card")) {
    if(requiredCard == "System Card 1.0") {
      hucardBiosId = 0;
    } else if(requiredCard == "Games Express") {
      hucardBiosId = 3;
    }
  }

  // If a hucard was required, mount it
  hucard = mia::Medium::create("PC Engine");
  if (hucardBiosId) {
    result = hucard->load(firmware[hucardBiosId.get()].location);
    if(result != successful) {
      result.firmwareSystemName = "PC Engine";
      result.firmwareType = firmware[hucardBiosId.get()].type;
      result.firmwareRegion = firmware[hucardBiosId.get()].region;
      result.result = noFirmware;
      return result;
    }
  }


  ares::PCEngine::option("Pixel Accuracy", settings.video.pixelAccuracy);

  auto name = "[Pioneer] LaserActive (NEC PAC)";
  if(!ares::PCEngine::load(root, {name, " (", region, ")"})) return otherError;

  if(hucardBiosId) {
    if (auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
      port->allocate();
      port->connect();
    }
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
  hucard->save(game->location);
  game->save(game->location);
  return true;
}

auto PCEngineLD::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "PC Engine") return system->pak;
  if(node->name() == "PC Engine Card") return hucard->pak;
  if(node->name() == "PC Engine CD Disc") return game->pak;
  if(node->name() == "Laserdisc") return game->pak;
  return {};
}
