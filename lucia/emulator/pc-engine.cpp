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
  shared_pointer<vfs::directory> bios;
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
  auto region = Emulator::region();
  string system = region == "NTSC-J" ? "PC Engine" : "TurboGrafx 16";
  if(!ares::PCEngine::load(root, {"[NEC] ", system, " (", region, ")"})) return false;

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
  if(node->name() == "PC Engine") {
    if(auto fp = pak->find(name)) return fp;
    if(auto fp = Emulator::save(name, mode, "save.ram",   ".sav")) return fp;
    if(auto fp = Emulator::save(name, mode, "backup.ram", ".brm")) return fp;
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
  auto region = Emulator::region();
  auto system = region == "NTSC-J" ? "PC Engine" : "TurboGrafx 16";
  //if statements below are ordered by lowest to highest priority
  if(region == "NTSC-J") regionID = 1;
  if(region == "NTSC-U") regionID = 0;
  if(!file::exists(firmware[regionID].location)) {
    errorFirmwareRequired(firmware[regionID]);
    return false;
  }
  bios = mia::medium("PC Engine")->pak(firmware[regionID].location);
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

auto PCEngineCD::open(ares::Node::Object node, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> {
  if(node->name() == "PC Engine") {
    if(auto fp = bios->find(name)) return fp;
    if(auto fp = Emulator::save(name, mode, "backup.ram", ".brm")) return fp;
  }
  if(node->name() == "PC Engine CD") {
    if(auto fp = pak->find(name)) return fp;
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
  if(!ares::PCEngine::load(root, "[NEC] SuperGrafx (NTSC-J)")) return false;

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
  if(node->name() == "SuperGrafx") {
    if(auto fp = pak->find(name)) return fp;
    if(auto fp = Emulator::save(name, mode, "save.ram", ".sav")) return fp;
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
