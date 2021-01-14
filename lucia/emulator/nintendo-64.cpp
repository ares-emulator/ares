namespace ares::Nintendo64 {
  auto load(Node::System& node, string name) -> bool;
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
  if(!ares::Nintendo64::load(root, "Nintendo 64")) return false;

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

  if(name == "program.rom") {
    return vfs::memory::open(game.image.data(), programROMSize);
  }

  return {};
}

auto Nintendo64::input(ares::Node::Input::Input node) -> void {
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
      button->setValue(value);
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
  if(!ares::Nintendo64::load(root, "Nintendo 64")) return false;

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
  if(name == "X-axis" ) mapping = virtualPads[0].lx;
  if(name == "Y-axis" ) mapping = virtualPads[0].ly;
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
      button->setValue(value);
    }
  }
}
