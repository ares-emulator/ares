struct PCEngine : Emulator {
  PCEngine();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
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

auto PCEngine::save() -> bool {
  root->save();
  medium->save(game.location, game.pak);
  return true;
}

auto PCEngine::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "PC Engine") return system.pak;
  if(node->name() == "PC Engine Card") return game.pak;
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
