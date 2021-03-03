struct GameBoy : Emulator {
  GameBoy();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;
};

GameBoy::GameBoy() {
  medium = mia::medium("Game Boy");
  manufacturer = "Nintendo";
  name = "Game Boy";
}

auto GameBoy::load() -> bool {
  system.pak->append("boot.dmg-1.rom", {Resource::GameBoy::BootDMG1, sizeof Resource::GameBoy::BootDMG1});

  if(!ares::GameBoy::load(root, "[Nintendo] Game Boy")) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto fastBoot = root->find<ares::Node::Setting::Boolean>("Fast Boot")) {
    fastBoot->setValue(settings.boot.fast);
  }

  return true;
}

auto GameBoy::save() -> bool {
  root->save();
  medium->save(game.location, game.pak);
  return true;
}

auto GameBoy::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Game Boy") return system.pak;
  if(node->name() == "Game Boy Cartridge") return game.pak;
  return {};
}

auto GameBoy::input(ares::Node::Input::Input node) -> void {
  auto name = node->name();
  maybe<InputMapping&> mapping;
  if(name == "Up"    ) mapping = virtualPads[0].up;
  if(name == "Down"  ) mapping = virtualPads[0].down;
  if(name == "Left"  ) mapping = virtualPads[0].left;
  if(name == "Right" ) mapping = virtualPads[0].right;
  if(name == "B"     ) mapping = virtualPads[0].a;
  if(name == "A"     ) mapping = virtualPads[0].b;
  if(name == "Select") mapping = virtualPads[0].select;
  if(name == "Start" ) mapping = virtualPads[0].start;
  //MBC5
  if(name == "Rumble") mapping = virtualPads[0].rumble;
  //MBC7
  if(name == "X"     ) mapping = virtualPads[0].lx;
  if(name == "Y"     ) mapping = virtualPads[0].ly;

  if(mapping) {
    auto value = mapping->value();
    if(auto axis = node->cast<ares::Node::Input::Axis>()) {
      axis->setValue(value);
    }
    if(auto button = node->cast<ares::Node::Input::Button>()) {
      button->setValue(value);
    }
    if(auto rumble = node->cast<ares::Node::Input::Rumble>()) {
      if(auto target = dynamic_cast<InputRumble*>(mapping.data())) {
        target->rumble(rumble->enable());
      }
    }
  }
}
