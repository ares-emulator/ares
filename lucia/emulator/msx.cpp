struct MSX : Emulator {
  MSX();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;
};

MSX::MSX() {
  manufacturer = "Microsoft";
  name = "MSX";
}

auto MSX::load() -> bool {
  game = mia::Medium::create("MSX");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  system = mia::System::create("MSX");
  if(!system->load()) return false;

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

auto MSX::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto MSX::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "MSX") return system->pak;
  if(node->name() == "MSX Cartridge") return game->pak;
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
