struct Saturn : Emulator {
  Saturn();
  auto load() -> LoadResult override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> std::shared_ptr<vfs::directory> override;

  u32 regionID = 0;
};

Saturn::Saturn() {
  manufacturer = "Sega";
  name = "Saturn";

  firmware.push_back({"BIOS", "US"});      //NTSC-U
  firmware.push_back({"BIOS", "Japan"});   //NTSC-J
  firmware.push_back({"BIOS", "Europe"});  //PAL
}

auto Saturn::load() -> LoadResult {
  game = mia::Medium::create("Saturn");
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;

  auto region = Emulator::region();
  //if statements below are ordered by lowest to highest priority
  if(region == "PAL"   ) regionID = 2;
  if(region == "NTSC-J") regionID = 1;
  if(region == "NTSC-U") regionID = 0;

  system = mia::System::create("Saturn");
  result = system->load(firmware[regionID].location);
  if(result != successful) {
    result.firmwareSystemName = "Saturn";
    result.firmwareType = firmware[regionID].type;
    result.firmwareRegion = firmware[regionID].region;
    result.result = noFirmware;
    return result;
  }

  if(!ares::Saturn::load(root, {"[Sega] Saturn (", region, ")"})) return otherError;

  if(auto port = root->find<ares::Node::Port>("Saturn/Disc Tray")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    port->allocate("Gamepad");
    port->connect();
  }

  return true;
}

auto Saturn::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto Saturn::pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> {
  if(node->name() == "Saturn") return system->pak;
  if(node->name() == "Saturn Disc") return game->pak;
  return {};
}
