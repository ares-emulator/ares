struct Famicom : Emulator {
  Famicom();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;
};

Famicom::Famicom() {
  manufacturer = "Nintendo";
  name = "Famicom";
}

auto Famicom::load() -> bool {
  game = mia::Medium::create("Famicom");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  system = mia::System::create("Famicom");
  if(!system->load()) return false;

  auto region = Emulator::region();
  if(!ares::Famicom::load(root, {"[Nintendo] Famicom (", region, ")"})) return false;

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

auto Famicom::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto Famicom::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Famicom") return system->pak;
  if(node->name() == "Famicom Cartridge") return game->pak;
  return {};
}

auto Famicom::input(ares::Node::Input::Input node) -> void {
  auto name = node->name();
  maybe<InputMapping&> mapping;
  if(name == "Up"        ) mapping = virtualPads[0].up;
  if(name == "Down"      ) mapping = virtualPads[0].down;
  if(name == "Left"      ) mapping = virtualPads[0].left;
  if(name == "Right"     ) mapping = virtualPads[0].right;
  if(name == "B"         ) mapping = virtualPads[0].a;
  if(name == "A"         ) mapping = virtualPads[0].b;
  if(name == "Select"    ) mapping = virtualPads[0].select;
  if(name == "Start"     ) mapping = virtualPads[0].start;
  if(name == "Microphone") mapping = virtualPads[0].x;

  if(mapping) {
    auto value = mapping->value();
    if(auto button = node->cast<ares::Node::Input::Button>()) {
       button->setValue(value);
    }
  }
}
