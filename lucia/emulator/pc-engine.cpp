namespace ares::PCEngine {
  auto load(Node::System& node, string name) -> bool;
}

struct PCEngine : Emulator {
  PCEngine();
  auto load() -> bool override;
  auto open(ares::Node::Object, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> override;
  auto input(ares::Node::Input::Input) -> void override;
};

struct PCEngineCD : Emulator {
  PCEngineCD();
  auto load() -> bool override;
  auto open(ares::Node::Object, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> override;
  auto input(ares::Node::Input::Input) -> void override;

  u32 regionID = 0;
};

struct SuperGrafx : Emulator {
  SuperGrafx();
  auto load() -> bool override;
  auto open(ares::Node::Object, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> override;
  auto input(ares::Node::Input::Input) -> void override;
};

PCEngine::PCEngine() {
  medium = mia::medium("PC Engine");
  manufacturer = "NEC";
  name = "PC Engine";
}

auto PCEngine::load() -> bool {
  if(!ares::PCEngine::load(root, "PC Engine")) return false;

  if(auto region = root->find<ares::Node::Setting::String>("Region")) {
    if(settings.boot.prefer != "NTSC-J") region->setValue("NTSC-U → NTSC-J");
    if(settings.boot.prefer == "NTSC-J") region->setValue("NTSC-J → NTSC-U");
  }

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port")) {
    port->allocate("Gamepad");
    port->connect();
  }

  return true;
}

auto PCEngine::open(ares::Node::Object node, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> {
  if(name == "manifest.bml") return Emulator::manifest();

  auto document = BML::unserialize(game.manifest);
  auto programROMSize = document["game/board/memory(content=Program,type=ROM)/size"].natural();
  auto programRAMVolatile = (bool)document["game/board/memory(content=Program,type=RAM)/volatile"];

  if(name == "program.rom") {
    return vfs::memory::open(game.image.data(), programROMSize);
  }

  if(name == "save.ram" && !programRAMVolatile) {
    auto location = locate(game.location, ".sav", settings.paths.saves);
    if(auto result = vfs::disk::open(location, mode)) return result;
  }

  if(name == "backup.ram") {
    auto location = locate(game.location, ".brm", settings.paths.saves);
    if(auto result = vfs::disk::open(location, mode)) return result;
  }

  return {};
}

auto PCEngine::input(ares::Node::Input::Input node) -> void {
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

PCEngineCD::PCEngineCD() {
  medium = mia::medium("PC Engine CD");
  manufacturer = "NEC";
  name = "PC Engine CD";

  firmware.append({"BIOS", "US"});     //NTSC-U
  firmware.append({"BIOS", "Japan"});  //NTSC-J
}

auto PCEngineCD::load() -> bool {
  if(!ares::PCEngine::load(root, "PC Engine")) return false;

  if(auto region = root->find<ares::Node::Setting::String>("Region")) {
    if(settings.boot.prefer != "NTSC-J") region->setValue("NTSC-U → NTSC-J"), regionID = 0;
    if(settings.boot.prefer == "NTSC-J") region->setValue("NTSC-J → NTSC-U"), regionID = 1;
  }

  if(auto manifest = medium->manifest(game.location)) {
    auto document = BML::unserialize(manifest);
    auto region = document["game/region"].string();
    //if statements below are ordered by lowest to highest priority
    if(region == "NTSC-J") regionID = 1;
    if(region == "NTSC-U") regionID = 0;
  }

  if(!file::exists(firmware[regionID].location)) {
    errorFirmwareRequired(firmware[regionID]);
    return false;
  }

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

auto PCEngineCD::open(ares::Node::Object node, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> {
  if(node->name() == "PC Engine") {
    if(name == "manifest.bml") {
      return Emulator::manifest("PC Engine", firmware[regionID].location);
    }

    if(name == "program.rom") {
      return Emulator::loadFirmware(firmware[regionID]);
    }

    if(name == "backup.ram") {
      auto location = locate(game.location, ".brm", settings.paths.saves);
      if(auto result = vfs::disk::open(location, mode)) return result;
    }
  }

  if(node->name() == "PC Engine CD") {
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

SuperGrafx::SuperGrafx() {
  medium = mia::medium("SuperGrafx");
  manufacturer = "NEC";
  name = "SuperGrafx";
}

auto SuperGrafx::load() -> bool {
  if(!ares::PCEngine::load(root, "SuperGrafx")) return false;

  if(auto region = root->find<ares::Node::Setting::String>("Region")) {
    if(settings.boot.prefer != "NTSC-J") region->setValue("NTSC-U → NTSC-J");
    if(settings.boot.prefer == "NTSC-J") region->setValue("NTSC-J → NTSC-U");
  }

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port")) {
    port->allocate("Gamepad");
    port->connect();
  }

  return true;
}

auto SuperGrafx::open(ares::Node::Object node, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> {
  if(name == "manifest.bml") return Emulator::manifest();

  auto document = BML::unserialize(game.manifest);
  auto programROMSize = document["game/board/memory(content=Program,type=ROM)/size"].natural();
  auto programRAMVolatile = (bool)document["game/board/memory(content=Program,type=RAM)/volatile"];

  if(name == "program.rom") {
    return vfs::memory::open(game.image.data(), programROMSize);
  }

  if(name == "save.ram" && !programRAMVolatile) {
    auto location = locate(game.location, ".sav", settings.paths.saves);
    if(auto result = vfs::disk::open(location, mode)) return result;
  }

  if(name == "backup.ram") {
    auto location = locate(game.location, ".brm", settings.paths.saves);
    if(auto result = vfs::disk::open(location, mode)) return result;
  }

  return {};
}

auto SuperGrafx::input(ares::Node::Input::Input node) -> void {
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
