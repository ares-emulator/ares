struct Saturn : Emulator {
  Saturn();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;

  u32 regionID = 0;
};

Saturn::Saturn() {
  manufacturer = "Sega";
  name = "Saturn";

  firmware.append({"BIOS", "US"});      //NTSC-U
  firmware.append({"BIOS", "Japan"});   //NTSC-J
  firmware.append({"BIOS", "Europe"});  //PAL
}

auto Saturn::load() -> bool {
  game = mia::Medium::create("Saturn");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  auto region = Emulator::region();
  //if statements below are ordered by lowest to highest priority
  if(region == "PAL"   ) regionID = 2;
  if(region == "NTSC-J") regionID = 1;
  if(region == "NTSC-U") regionID = 0;

  system = mia::System::create("Saturn");
  if(!system->load(firmware[regionID].location)) return errorFirmware(firmware[regionID]), false;

  if(!ares::Saturn::load(root, {"[Sega] Saturn (", region, ")"})) return false;

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

auto Saturn::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Saturn") return system->pak;
  if(node->name() == "Saturn Disc") return game->pak;
  return {};
}
