namespace ares::MegaDrive {
  auto load(Node::System& node, string name) -> bool;
}

struct MegaDrive : Emulator {
  MegaDrive();
  auto load() -> bool override;
  auto open(ares::Node::Object, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> override;
  auto input(ares::Node::Input::Input) -> void override;
};

struct MegaCD : Emulator {
  MegaCD();
  auto load() -> bool override;
  auto open(ares::Node::Object, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> override;
  auto input(ares::Node::Input::Input) -> void override;

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

auto MegaDrive::open(ares::Node::Object node, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> {
  if(name == "manifest.bml") return Emulator::manifest();

  auto document = BML::unserialize(game.manifest);
  auto programROMSize = document["game/board/memory(content=Program,type=ROM)/size"].natural();
  auto saveRAMVolatile = (bool)document["game/board/memory(Content=Save,type=RAM)/volatile"];

  if(name == "program.rom") {
    return vfs::memory::open(game.image.data(), programROMSize);
  }

  if(name == "save.ram" && !saveRAMVolatile) {
    auto location = locate(game.location, ".sav", settings.paths.saves);
    if(auto result = vfs::disk::open(location, mode)) return result;
  }

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
  if(!ares::MegaDrive::load(root, {"[Sega] ", system, " (", region, ")"})) return false;

  if(!file::exists(firmware[regionID].location)) {
    errorFirmwareRequired(firmware[regionID]);
    return false;
  }

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

auto MegaCD::open(ares::Node::Object node, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> {
  if(node->name() == "Mega Drive") {
    if(name == "manifest.bml") {
      return Emulator::manifest("Mega Drive", firmware[regionID].location);
    }

    if(name == "program.rom") {
      return Emulator::loadFirmware(firmware[regionID]);
    }

    if(name == "backup.ram") {
      auto location = locate(game.location, ".sav", settings.paths.saves);
      if(auto result = vfs::disk::open(location, mode)) return result;
    }
  }

  if(node->name() == "Mega CD") {
    if(name == "manifest.bml") {
      if(auto manifest = medium->manifest(game.location)) {
        return vfs::memory::open(manifest.data<u8>(), manifest.size());
      }
      return Emulator::manifest(game.location);
    }

    if(name == "cd.rom") {
      if(game.location.iendsWith(".zip")) {
        MessageDialog().setText(
          "Sorry, compressed CD-ROM images are not currently supported.\n"
          "Please extract the image prior to loading it."
        ).setAlignment(presentation).error();
        return {};
      }

      if(auto result = vfs::cdrom::open(game.location)) return result;

      MessageDialog().setText(
        "Failed to load CD-ROM image."
      ).setAlignment(presentation).error();
    }
  }

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
