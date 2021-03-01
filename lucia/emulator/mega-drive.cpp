namespace ares::MegaDrive {
  auto load(Node::System& node, string name) -> bool;
}

struct MegaDrive : Emulator {
  MegaDrive();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;
};

struct MegaCD : Emulator {
  MegaCD();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;

  Pak bios;
  u32 regionID = 0;
};

MegaDrive::MegaDrive() {
  medium = mia::medium("Mega Drive");
  manufacturer = "Sega";
  name = "Mega Drive";
}

auto MegaDrive::load() -> bool {
  auto region = Emulator::region();
  auto system = region == "NTSC-U" ? "Genesis" : "Mega Drive";
  if(!ares::MegaDrive::load(root, {"[Sega] ", system, " (", region, ")"})) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    port->allocate("Fighting Pad");
    port->connect();
  }

  return true;
}

auto MegaDrive::save() -> bool {
  root->save();
  return medium->save(game.location, game.pak);
}

auto MegaDrive::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Mega Drive") return game.pak;
  return {};
}

auto MegaDrive::input(ares::Node::Input::Input node) -> void {
  auto name = node->name();
  maybe<InputMapping&> mapping;
  if(name == "Up"   ) mapping = virtualPads[0].up;
  if(name == "Down" ) mapping = virtualPads[0].down;
  if(name == "Left" ) mapping = virtualPads[0].left;
  if(name == "Right") mapping = virtualPads[0].right;
  if(name == "A"    ) mapping = virtualPads[0].a;
  if(name == "B"    ) mapping = virtualPads[0].b;
  if(name == "C"    ) mapping = virtualPads[0].c;
  if(name == "X"    ) mapping = virtualPads[0].x;
  if(name == "Y"    ) mapping = virtualPads[0].y;
  if(name == "Z"    ) mapping = virtualPads[0].z;
  if(name == "Mode" ) mapping = virtualPads[0].select;
  if(name == "Start") mapping = virtualPads[0].start;

  if(mapping) {
    auto value = mapping->value();
    if(auto button = node->cast<ares::Node::Input::Button>()) {
      button->setValue(value);
    }
  }
}

MegaCD::MegaCD() {
  medium = mia::medium("Mega CD");
  manufacturer = "Sega";
  name = "Mega CD";

  firmware.append({"BIOS", "US"});      //NTSC-U
  firmware.append({"BIOS", "Japan"});   //NTSC-J
  firmware.append({"BIOS", "Europe"});  //PAL
}

auto MegaCD::load() -> bool {
  auto region = Emulator::region();
  auto system = region == "NTSC-U" ? "Genesis" : "Mega Drive";
  //if statements below are ordered by lowest to highest priority
  if(region == "PAL"   ) regionID = 2;
  if(region == "NTSC-J") regionID = 1;
  if(region == "NTSC-U") regionID = 0;

  if(!file::exists(firmware[regionID].location)) {
    errorFirmwareRequired(firmware[regionID]);
    return false;
  }
  bios.pak = mia::medium("Mega Drive")->load(firmware[regionID].location);

  if(!ares::MegaDrive::load(root, {"[Sega] ", system, " (", region, ")"})) return false;

  if(auto port = root->find<ares::Node::Port>("Expansion Port")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->scan<ares::Node::Port>("Disc Tray")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    port->allocate("Fighting Pad");
    port->connect();
  }

  return true;
}

auto MegaCD::save() -> bool {
  root->save();
  return medium->save(game.location, game.pak);
}

auto MegaCD::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Mega Drive") return bios.pak;
  if(node->name() == "Mega CD") return game.pak;
  return {};
}

auto MegaCD::input(ares::Node::Input::Input node) -> void {
  auto name = node->name();
  maybe<InputMapping&> mapping;
  if(name == "Up"   ) mapping = virtualPads[0].up;
  if(name == "Down" ) mapping = virtualPads[0].down;
  if(name == "Left" ) mapping = virtualPads[0].left;
  if(name == "Right") mapping = virtualPads[0].right;
  if(name == "A"    ) mapping = virtualPads[0].a;
  if(name == "B"    ) mapping = virtualPads[0].b;
  if(name == "C"    ) mapping = virtualPads[0].c;
  if(name == "X"    ) mapping = virtualPads[0].x;
  if(name == "Y"    ) mapping = virtualPads[0].y;
  if(name == "Z"    ) mapping = virtualPads[0].z;
  if(name == "Mode" ) mapping = virtualPads[0].select;
  if(name == "Start") mapping = virtualPads[0].start;

  if(mapping) {
    auto value = mapping->value();
    if(auto button = node->cast<ares::Node::Input::Button>()) {
      button->setValue(value);
    }
  }
}
