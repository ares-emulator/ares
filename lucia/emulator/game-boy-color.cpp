struct GameBoyColor : Emulator {
  GameBoyColor();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;
};

GameBoyColor::GameBoyColor() {
  manufacturer = "Nintendo";
  name = "Game Boy Color";
}

auto GameBoyColor::load() -> bool {
  game = mia::Medium::create("Game Boy Color");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  system = mia::System::create("Game Boy Color");
  if(!system->load()) return false;

  if(!ares::GameBoy::load(root, "[Nintendo] Game Boy Color")) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto fastBoot = root->find<ares::Node::Setting::Boolean>("Fast Boot")) {
    fastBoot->setValue(settings.boot.fast);
  }

  return true;
}

auto GameBoyColor::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto GameBoyColor::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Game Boy Color") return system->pak;
  if(node->name() == "Game Boy Color Cartridge") return game->pak;
  return {};
}

auto GameBoyColor::input(ares::Node::Input::Input node) -> void {
  auto name = node->name();
  maybe<InputMapping&> mappings[2];
  if(name == "Up"    ) mappings[0] = virtualPads[0].up;
  if(name == "Down"  ) mappings[0] = virtualPads[0].down;
  if(name == "Left"  ) mappings[0] = virtualPads[0].left;
  if(name == "Right" ) mappings[0] = virtualPads[0].right;
  if(name == "B"     ) mappings[0] = virtualPads[0].a;
  if(name == "A"     ) mappings[0] = virtualPads[0].b;
  if(name == "Select") mappings[0] = virtualPads[0].select;
  if(name == "Start" ) mappings[0] = virtualPads[0].start;
  //MBC5
  if(name == "Rumble") mappings[0] = virtualPads[0].rumble;
  //MBC7
  if(name == "X"     ) mappings[0] = virtualPads[0].lleft, mappings[1] = virtualPads[0].lright;
  if(name == "Y"     ) mappings[0] = virtualPads[0].lup,   mappings[1] = virtualPads[0].ldown;

  if(mappings[0]) {
    if(auto axis = node->cast<ares::Node::Input::Axis>()) {
      auto value = mappings[1]->value() - mappings[0]->value();
      axis->setValue(value);
    }
    if(auto button = node->cast<ares::Node::Input::Button>()) {
      auto value = mappings[0]->value();
      button->setValue(value);
    }
    if(auto rumble = node->cast<ares::Node::Input::Rumble>()) {
      if(auto target = dynamic_cast<InputRumble*>(mappings[0].data())) {
        target->rumble(rumble->enable());
      }
    }
  }
}
