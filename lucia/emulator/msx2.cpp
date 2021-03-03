struct MSX2 : Emulator {
  MSX2();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;
};

MSX2::MSX2() {
  medium = mia::medium("MSX2");
  manufacturer = "Microsoft";
  name = "MSX2";
}

auto MSX2::load() -> bool {
  system.pak->append("bios.rom", {Resource::MSX2::BIOS, sizeof Resource::MSX2::BIOS});
  system.pak->append("sub.rom",  {Resource::MSX2::Sub,  sizeof Resource::MSX2::Sub });

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

auto MSX2::save() -> bool {
  root->save();
  medium->save(game.location, game.pak);
  return true;
}

auto MSX2::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "MSX2") return system.pak;
  if(node->name() == "MSX2 Cartridge") return game.pak;
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
