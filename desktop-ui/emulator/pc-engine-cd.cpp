struct PCEngineCD : PCEngine {
  PCEngineCD();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;

  shared_pointer<mia::Pak> bios;
  u32 regionID = 0;
};

PCEngineCD::PCEngineCD() {
  manufacturer = "NEC";
  name = "PC Engine CD";

  firmware.append({"BIOS", "US", "cadac2725711b3c442bcf237b02f5a5210c96f17625c35fa58f009e0ed39e4db"});     //NTSC-U
  firmware.append({"BIOS", "Japan", "e11527b3b96ce112a037138988ca72fd117a6b0779c2480d9e03eaebece3d9ce"});  //NTSC-J

  allocatePorts();
}

auto PCEngineCD::load() -> bool {
  game = mia::Medium::create("PC Engine CD");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  auto region = Emulator::region();
  //if statements below are ordered by lowest to highest priority
  if(region == "NTSC-J") regionID = 1;
  if(region == "NTSC-U") regionID = 0;

  bios = mia::Medium::create("PC Engine");
  if(!bios->load(firmware[regionID].location)) return errorFirmware(firmware[regionID]), false;

  system = mia::System::create("PC Engine");
  if(!system->load()) return false;

  ares::PCEngine::option("Pixel Accuracy", settings.video.pixelAccuracy);

  auto name = region == "NTSC-J" ? "PC Engine" : "TurboGrafx 16";
  if(!ares::PCEngine::load(root, {"[NEC] ", name, " (", region, ")"})) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("PC Engine CD/Disc Tray")) {
    port->allocate();
    port->connect();
  }

  connectPorts();

  return true;
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
