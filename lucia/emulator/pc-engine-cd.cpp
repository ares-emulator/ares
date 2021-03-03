struct PCEngineCD : Emulator {
  PCEngineCD();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;

  Pak bios;
  u32 regionID = 0;
};

PCEngineCD::PCEngineCD() {
  medium = mia::medium("PC Engine CD");
  manufacturer = "NEC";
  name = "PC Engine CD";

  firmware.append({"BIOS", "US"});     //NTSC-U
  firmware.append({"BIOS", "Japan"});  //NTSC-J
}

auto PCEngineCD::load() -> bool {
  auto region = Emulator::region();
  auto system = region == "NTSC-J" ? "PC Engine" : "TurboGrafx 16";
  //if statements below are ordered by lowest to highest priority
  if(region == "NTSC-J") regionID = 1;
  if(region == "NTSC-U") regionID = 0;

  if(!file::exists(firmware[regionID].location)) {
    errorFirmwareRequired(firmware[regionID]);
    return false;
  }
  medium->load(game.location, this->system.pak, "backup.ram", ".bram", 2_KiB);
  bios.pak = mia::medium("PC Engine")->load(firmware[regionID].location);

  if(!ares::PCEngine::load(root, {"[NEC] ", system, " (", region, ")"})) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("PC Engine CD/Disc Tray")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port")) {
    port->allocate("Gamepad");
    port->connect();
  }

  return true;
}

auto PCEngineCD::save() -> bool {
  root->save();
  medium->save(game.location, system.pak, "backup.ram", ".bram");
  medium->save(game.location, game.pak);
  return true;
}

auto PCEngineCD::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "PC Engine") return system.pak;
  if(node->name() == "PC Engine Card") return bios.pak;
  if(node->name() == "PC Engine CD Disc") return game.pak;
  return {};
}

auto PCEngineCD::input(ares::Node::Input::Input node) -> void {
  auto name = node->name();
  maybe<InputMapping&> mapping;
  if(name == "Up"    ) mapping = virtualPads[0].up;
  if(name == "Down"  ) mapping = virtualPads[0].down;
  if(name == "Left"  ) mapping = virtualPads[0].left;
  if(name == "Right" ) mapping = virtualPads[0].right;
  if(name == "II"    ) mapping = virtualPads[0].a;
  if(name == "I"     ) mapping = virtualPads[0].b;
  if(name == "Select") mapping = virtualPads[0].select;
  if(name == "Run"   ) mapping = virtualPads[0].start;

  if(mapping) {
    auto value = mapping->value();
    if(auto button = node->cast<ares::Node::Input::Button>()) {
      button->setValue(value);
    }
  }
}
