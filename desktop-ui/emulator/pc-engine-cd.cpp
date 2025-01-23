struct PCEngineCD : PCEngine {
  PCEngineCD();
  auto load() -> LoadResult override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;

  shared_pointer<mia::Pak> bios;
  u32 biosID = 0;
};

PCEngineCD::PCEngineCD() {
  manufacturer = "NEC";
  name = "PC Engine CD";

  firmware.append({"System-Card 1.0", "Japan", "afe9f27f91ac918348555b86298b4f984643eafa2773196f2c5441ea84f0c3bb"});
  firmware.append({"Arcade Card"    , "Japan", "e11527b3b96ce112a037138988ca72fd117a6b0779c2480d9e03eaebece3d9ce"});
  firmware.append({"System Card 3.0", "US",    "cadac2725711b3c442bcf237b02f5a5210c96f17625c35fa58f009e0ed39e4db"});
  firmware.append({"Games Express"  , "Japan", "4b86bb96a48a4ca8375fc0109631d0b1d64f255a03b01de70594d40788ba6c3d"});

  allocatePorts();
}

auto PCEngineCD::load() -> LoadResult {
  game = mia::Medium::create("PC Engine CD");
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;

  auto region = Emulator::region();
  //if statements below are ordered by lowest to highest priority
  if(region == "NTSC-U") biosID = 2;
  if(region == "NTSC-J") biosID = 1;

  // Some games require a specific card to be inserted into the system
  if(auto requiredCard = game->pak->attribute("card")) {
    if(requiredCard == "System Card 1.0") {
      biosID = 0;
    } else if(requiredCard == "Games Express") {
      biosID = 3;
    }
  }

  bios = mia::Medium::create("PC Engine");
  result = bios->load(firmware[biosID].location);
  if(result != successful) {
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

  auto name = region == "NTSC-J" ? "PC Engine Duo" : "TurboDuo";
  if(!ares::PCEngine::load(root, {"[NEC] ", name, " (", region, ")"})) return otherError;

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

auto PCEngineCD::save() -> bool {
  root->save();
  system->save(game->location);
  bios->save(game->location);
  game->save(game->location);
  return true;
}

auto PCEngineCD::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "PC Engine") return system->pak;
  if(node->name() == "PC Engine Card") return bios->pak;
  if(node->name() == "PC Engine CD Disc") return game->pak;
  return {};
}
