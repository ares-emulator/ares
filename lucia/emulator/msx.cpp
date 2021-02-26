namespace ares::MSX {
  auto load(Node::System& node, string name) -> bool;
}

struct MSX : Emulator {
  MSX();
  auto load() -> bool override;
  auto open(ares::Node::Object, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> override;
  auto input(ares::Node::Input::Input) -> void override;
};

struct MSX2 : Emulator {
  MSX2();
  auto load() -> bool override;
  auto open(ares::Node::Object, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> override;
  auto input(ares::Node::Input::Input) -> void override;
};

MSX::MSX() {
  medium = mia::medium("MSX");
  manufacturer = "Microsoft";
  name = "MSX";
}

auto MSX::load() -> bool {
  auto region = Emulator::region();
  if(!ares::MSX::load(root, {"[Microsoft] MSX (", region, ")"})) return false;

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

auto MSX::open(ares::Node::Object node, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> {
  if(name == "bios.rom") return vfs::memory::open({Resource::MSX::BIOS, sizeof Resource::MSX::BIOS});
  if(node->name() == "MSX") {
    if(auto fp = pak->find(name)) return fp;
  }
  return {};
}

auto MSX::input(ares::Node::Input::Input node) -> void {
  auto name = node->name();
  maybe<InputMapping&> mapping;
  if(name == "Up"   ) mapping = virtualPads[0].up;
  if(name == "Down" ) mapping = virtualPads[0].down;
  if(name == "Left" ) mapping = virtualPads[0].left;
  if(name == "Right") mapping = virtualPads[0].right;
  if(name == "A"    ) mapping = virtualPads[0].a;
  if(name == "B"    ) mapping = virtualPads[0].b;

  if(mapping) {
    auto value = mapping->value();
    if(auto button = node->cast<ares::Node::Input::Button>()) {
      button->setValue(value);
    }
  }
}

MSX2::MSX2() {
  medium = mia::medium("MSX2");
  manufacturer = "Microsoft";
  name = "MSX2";
}

auto MSX2::load() -> bool {
  auto region = Emulator::region();
  if(!ares::MSX::load(root, {"[Microsoft] MSX2 (", region, ")"})) return false;

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

auto MSX2::open(ares::Node::Object node, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> {
  if(name == "bios.rom") return vfs::memory::open({Resource::MSX2::BIOS, sizeof Resource::MSX2::BIOS});
  if(node->name() == "MSX2") {
    if(auto fp = pak->find(name)) return fp;
  }
  return {};
}

auto MSX2::input(ares::Node::Input::Input node) -> void {
  auto name = node->name();
  maybe<InputMapping&> mapping;
  if(name == "Up"   ) mapping = virtualPads[0].up;
  if(name == "Down" ) mapping = virtualPads[0].down;
  if(name == "Left" ) mapping = virtualPads[0].left;
  if(name == "Right") mapping = virtualPads[0].right;
  if(name == "A"    ) mapping = virtualPads[0].a;
  if(name == "B"    ) mapping = virtualPads[0].b;

  if(mapping) {
    auto value = mapping->value();
    if(auto button = node->cast<ares::Node::Input::Button>()) {
      button->setValue(value);
    }
  }
}
