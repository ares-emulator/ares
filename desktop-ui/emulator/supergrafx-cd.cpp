struct SuperGrafxCD : PCEngine {
  SuperGrafxCD();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;

  shared_pointer<mia::Pak> bios;
  u32 regionID = 0;
};

SuperGrafxCD::SuperGrafxCD() {
  manufacturer = "NEC";
  name = "SuperGrafx CD";

  firmware.append({"BIOS", "Japan"});  //NTSC-J

  allocatePorts();
}

auto SuperGrafxCD::load() -> bool {
  game = mia::Medium::create("PC Engine CD");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  bios = mia::Medium::create("PC Engine");
  if(!bios->load(firmware[0].location)) return errorFirmware(firmware[0]), false;

  system = mia::System::create("SuperGrafx");
  if(!system->load()) return false;

  ares::PCEngine::option("Pixel Accuracy", settings.video.pixelAccuracy);

  if(!ares::PCEngine::load(root, "[NEC] SuperGrafx (NTSC-J)")) return false;

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

auto SuperGrafxCD::save() -> bool {
  root->save();
  system->save(game->location);
  bios->save(game->location);
  game->save(game->location);
  return true;
}

auto SuperGrafxCD::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "SuperGrafx") return system->pak;
  if(node->name() == "SuperGrafx Card") return bios->pak;
  if(node->name() == "PC Engine CD Disc") return game->pak;
  return {};
}
