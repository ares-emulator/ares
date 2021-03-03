struct SG1000 : Emulator {
  SG1000();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;
};

SG1000::SG1000() {
  medium = mia::medium("SG-1000");
  manufacturer = "Sega";
  name = "SG-1000";
}

auto SG1000::load() -> bool {
  auto region = Emulator::region();
  if(!ares::SG1000::load(root, {"[Sega] SG-1000 (", region, ")"})) return false;

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

auto SG1000::save() -> bool {
  root->save();
  medium->save(game.location, game.pak);
  return true;
}

auto SG1000::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "SG-1000 Cartridge") return game.pak;
  return {};
}

auto SG1000::input(ares::Node::Input::Input node) -> void {
  auto name = node->name();
  maybe<InputMapping&> mapping;
  if(name == "Up"   ) mapping = virtualPads[0].up;
  if(name == "Down" ) mapping = virtualPads[0].down;
  if(name == "Left" ) mapping = virtualPads[0].left;
  if(name == "Right") mapping = virtualPads[0].right;
  if(name == "1"    ) mapping = virtualPads[0].a;
  if(name == "2"    ) mapping = virtualPads[0].b;

  if(mapping) {
    auto value = mapping->value();
    if(auto button = node->cast<ares::Node::Input::Button>()) {
      button->setValue(value);
    }
  }
}
