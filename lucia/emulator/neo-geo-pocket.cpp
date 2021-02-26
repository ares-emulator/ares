namespace ares::NeoGeoPocket {
  auto load(Node::System& node, string name) -> bool;
}

struct NeoGeoPocket : Emulator {
  NeoGeoPocket();
  auto load() -> bool override;
  auto open(ares::Node::Object, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> override;
  auto input(ares::Node::Input::Input) -> void override;
};

struct NeoGeoPocketColor : Emulator {
  NeoGeoPocketColor();
  auto load() -> bool override;
  auto open(ares::Node::Object, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> override;
  auto input(ares::Node::Input::Input) -> void override;
};

NeoGeoPocket::NeoGeoPocket() {
  medium = mia::medium("Neo Geo Pocket");
  manufacturer = "SNK";
  name = "Neo Geo Pocket";

  firmware.append({"BIOS", "World", "0293555b21c4fac516d25199df7809b26beeae150e1d4504a050db32264a6ad7"});
}

auto NeoGeoPocket::load() -> bool {
  if(!ares::NeoGeoPocket::load(root, "[SNK] Neo Geo Pocket")) return false;

  if(!file::exists(firmware[0].location)) {
    errorFirmwareRequired(firmware[0]);
    return false;
  }

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto fastBoot = root->find<ares::Node::Setting::Boolean>("Fast Boot")) {
    fastBoot->setValue(settings.boot.fast);
  }

  return true;
}

auto NeoGeoPocket::open(ares::Node::Object node, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> {
  if(name == "bios.rom") return loadFirmware(firmware[0]);
  if(node->name() == "Neo Geo Pocket") {
    if(auto fp = Emulator::save(name, mode, "program.flash", ".sav")) return fp;
    if(auto fp = pak->find(name)) return fp;
  }
  return {};
}

auto NeoGeoPocket::input(ares::Node::Input::Input node) -> void {
  auto name = node->name();
  maybe<InputMapping&> mapping;
  if(name == "Up"    ) mapping = virtualPads[0].up;
  if(name == "Down"  ) mapping = virtualPads[0].down;
  if(name == "Left"  ) mapping = virtualPads[0].left;
  if(name == "Right" ) mapping = virtualPads[0].right;
  if(name == "A"     ) mapping = virtualPads[0].a;
  if(name == "B"     ) mapping = virtualPads[0].b;
  if(name == "Option") mapping = virtualPads[0].start;

  if(mapping) {
    auto value = mapping->value();
    if(auto button = node->cast<ares::Node::Input::Button>()) {
      button->setValue(value);
    }
  }
}

NeoGeoPocketColor::NeoGeoPocketColor() {
  medium = mia::medium("Neo Geo Pocket Color");
  manufacturer = "SNK";
  name = "Neo Geo Pocket Color";

  firmware.append({"BIOS", "World", "8fb845a2f71514cec20728e2f0fecfade69444f8d50898b92c2259f1ba63e10d"});
}

auto NeoGeoPocketColor::load() -> bool {
  if(!ares::NeoGeoPocket::load(root, "[SNK] Neo Geo Pocket Color")) return false;

  if(!file::exists(firmware[0].location)) {
    errorFirmwareRequired(firmware[0]);
    return false;
  }

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto fastBoot = root->find<ares::Node::Setting::Boolean>("Fast Boot")) {
    fastBoot->setValue(settings.boot.fast);
  }

  return true;
}

auto NeoGeoPocketColor::open(ares::Node::Object node, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> {
  if(name == "bios.rom") return loadFirmware(firmware[0]);
  if(node->name() == "Neo Geo Pocket Color") {
    if(auto fp = Emulator::save(name, mode, "program.flash", ".sav")) return fp;
    if(auto fp = pak->find(name)) return fp;
  }
  return {};
}

auto NeoGeoPocketColor::input(ares::Node::Input::Input node) -> void {
  auto name = node->name();
  maybe<InputMapping&> mapping;
  if(name == "Up"    ) mapping = virtualPads[0].up;
  if(name == "Down"  ) mapping = virtualPads[0].down;
  if(name == "Left"  ) mapping = virtualPads[0].left;
  if(name == "Right" ) mapping = virtualPads[0].right;
  if(name == "A"     ) mapping = virtualPads[0].a;
  if(name == "B"     ) mapping = virtualPads[0].b;
  if(name == "Option") mapping = virtualPads[0].start;

  if(mapping) {
    auto value = mapping->value();
    if(auto button = node->cast<ares::Node::Input::Button>()) {
      button->setValue(value);
    }
  }
}
