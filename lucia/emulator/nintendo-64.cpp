namespace ares::Nintendo64 {
  auto load(Node::System& node, string name) -> bool;
  auto option(string name, string value) -> bool;
}

struct Nintendo64 : Emulator {
  Nintendo64();
  auto load() -> bool override;
  auto open(ares::Node::Object, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> override;
  auto input(ares::Node::Input::Input) -> void override;
};

struct Nintendo64DD : Emulator {
  Nintendo64DD();
  auto load() -> bool override;
  auto open(ares::Node::Object, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> override;
  auto input(ares::Node::Input::Input) -> void override;
};

Nintendo64::Nintendo64() {
  medium = mia::medium("Nintendo 64");
  manufacturer = "Nintendo";
  name = "Nintendo 64";
}

auto Nintendo64::load() -> bool {
  ares::Nintendo64::option("Quality", settings.video.quality);
  ares::Nintendo64::option("Supersampling", settings.video.supersampling);

  auto region = Emulator::region();
  if(!ares::Nintendo64::load(root, {"[Nintendo] Nintendo 64 (", region, ")"})) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    auto peripheral = port->allocate("Gamepad");
    port->connect();
    if(auto port = peripheral->find<ares::Node::Port>("Pak")) {
      auto document = BML::unserialize(game.manifest);
      if(!document["game/board/memory(content=Save)"]) {
        port->allocate("Controller Pak");
        port->connect();
      } else {
        port->allocate("Rumble Pak");
        port->connect();
      }
    }
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 2")) {
    auto peripheral = port->allocate("Gamepad");
    port->connect();
    if(auto port = peripheral->find<ares::Node::Port>("Pak")) {
      port->allocate("Rumble Pak");
      port->connect();
    }
  }

  return true;
}

auto Nintendo64::open(ares::Node::Object node, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> {
  if(name == "manifest.bml") return Emulator::manifest();

  if(name == "pif.rom") {
    return vfs::memory::open(Resource::Nintendo64::PIF::ROM, sizeof Resource::Nintendo64::PIF::ROM);
  }

  if(name == "pif.ntsc.rom") {
    return vfs::memory::open(Resource::Nintendo64::PIF::NTSC, sizeof Resource::Nintendo64::PIF::NTSC);
  }

  if(name == "pif.pal.rom") {
    return vfs::memory::open(Resource::Nintendo64::PIF::PAL, sizeof Resource::Nintendo64::PIF::PAL);
  }

  auto document = BML::unserialize(game.manifest);
  auto programROMSize = document["game/board/memory(content=Program,type=ROM)/size"].natural();
  auto saveRAMVolatile = (bool)document["game/board/memory(content=Save,type=RAM)/volatile"];

  if(name == "program.rom") {
    return vfs::memory::open(game.image.data(), programROMSize);
  }

  if(name == "save.ram" && !saveRAMVolatile) {
    auto location = locate(game.location, ".sav", settings.paths.saves);
    if(auto result = vfs::disk::open(location, mode)) return result;
  }

  if(name == "save.eeprom") {
    auto location = locate(game.location, ".sav", settings.paths.saves);
    if(auto result = vfs::disk::open(location, mode)) return result;
  }

  if(name == "save.flash") {
    auto location = locate(game.location, ".sav", settings.paths.saves);
    if(auto result = vfs::disk::open(location, mode)) return result;
  }

  if(name == "save.pak") {
    auto location = locate(game.location, ".pak", settings.paths.saves);
    if(auto result = vfs::disk::open(location, mode)) return result;
  }

  return {};
}

auto Nintendo64::input(ares::Node::Input::Input node) -> void {
  auto parent = ares::Node::parent(node);
  if(!parent) return;

  auto port = ares::Node::parent(parent);
  if(!port) return;

  maybe<u32> index;
  if(port->name() == "Controller Port 1") index = 0;
  if(port->name() == "Controller Port 2") index = 1;
  if(!index) return;

  auto name = node->name();
  maybe<InputMapping&> mapping;
  if(name == "X-Axis" ) mapping = virtualPads[*index].lx;
  if(name == "Y-Axis" ) mapping = virtualPads[*index].ly;
  if(name == "Up"     ) mapping = virtualPads[*index].up;
  if(name == "Down"   ) mapping = virtualPads[*index].down;
  if(name == "Left"   ) mapping = virtualPads[*index].left;
  if(name == "Right"  ) mapping = virtualPads[*index].right;
  if(name == "B"      ) mapping = virtualPads[*index].a;
  if(name == "A"      ) mapping = virtualPads[*index].b;
  if(name == "C-Up"   ) mapping = virtualPads[*index].ry;
  if(name == "C-Down" ) mapping = virtualPads[*index].ry;
  if(name == "C-Left" ) mapping = virtualPads[*index].rx;
  if(name == "C-Right") mapping = virtualPads[*index].rx;
  if(name == "L"      ) mapping = virtualPads[*index].l1;
  if(name == "R"      ) mapping = virtualPads[*index].r1;
  if(name == "Z"      ) mapping = virtualPads[*index].z;
  if(name == "Start"  ) mapping = virtualPads[*index].start;
  if(name == "Rumble" ) mapping = virtualPads[*index].rumble;

  if(mapping) {
    auto value = mapping->value();
    if(auto axis = node->cast<ares::Node::Input::Axis>()) {
      axis->setValue(value);
    }
    if(auto button = node->cast<ares::Node::Input::Button>()) {
      if(name == "C-Up"   || name == "C-Left" ) return button->setValue(value < -16384);
      if(name == "C-Down" || name == "C-Right") return button->setValue(value > +16384);
      button->setValue(value);
    }
    if(auto rumble = node->cast<ares::Node::Input::Rumble>()) {
      if(auto target = dynamic_cast<InputRumble*>(mapping.data())) {
        target->rumble(rumble->enable());
      }
    }
  }
}

Nintendo64DD::Nintendo64DD() {
  medium = mia::medium("Nintendo 64DD");
  manufacturer = "Nintendo";
  name = "Nintendo 64DD";

  firmware.append({"BIOS", "Japan"});
}

auto Nintendo64DD::load() -> bool {
  ares::Nintendo64::option("Quality", settings.video.quality);
  ares::Nintendo64::option("Supersampling", settings.video.supersampling);

  auto region = Emulator::region();
  if(!ares::Nintendo64::load(root, {"[Nintendo] Nintendo 64 (", region, ")"})) return false;

  if(!file::exists(firmware[0].location)) {
    errorFirmwareRequired(firmware[0]);
    return false;
  }

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    port->allocate("Gamepad");
    port->connect();
  }

  return true;
}

auto Nintendo64DD::open(ares::Node::Object node, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> {
  if(node->name() == "Disk Drive") {
    if(name == "program.rom") {
      return loadFirmware(firmware[0]);
    }
  }

  if(node->name() == "Nintendo 64DD") {
  }

  return {};
}

auto Nintendo64DD::input(ares::Node::Input::Input node) -> void {
  auto name = node->name();
  maybe<InputMapping&> mapping;
  if(name == "X-Axis" ) mapping = virtualPads[0].lx;
  if(name == "Y-Axis" ) mapping = virtualPads[0].ly;
  if(name == "Up"     ) mapping = virtualPads[0].up;
  if(name == "Down"   ) mapping = virtualPads[0].down;
  if(name == "Left"   ) mapping = virtualPads[0].left;
  if(name == "Right"  ) mapping = virtualPads[0].right;
  if(name == "B"      ) mapping = virtualPads[0].a;
  if(name == "A"      ) mapping = virtualPads[0].b;
  if(name == "C-Up"   ) mapping = virtualPads[0].ry;
  if(name == "C-Down" ) mapping = virtualPads[0].ry;
  if(name == "C-Left" ) mapping = virtualPads[0].rx;
  if(name == "C-Right") mapping = virtualPads[0].rx;
  if(name == "L"      ) mapping = virtualPads[0].l1;
  if(name == "R"      ) mapping = virtualPads[0].r1;
  if(name == "Z"      ) mapping = virtualPads[0].z;
  if(name == "Start"  ) mapping = virtualPads[0].start;

  if(mapping) {
    auto value = mapping->value();
    if(auto axis = node->cast<ares::Node::Input::Axis>()) {
      axis->setValue(value);
    }
    if(auto button = node->cast<ares::Node::Input::Button>()) {
      if(name == "C-Up"   || name == "C-Left" ) return button->setValue(value < -16384);
      if(name == "C-Down" || name == "C-Right") return button->setValue(value > +16384);
      button->setValue(value);
    }
  }
}
