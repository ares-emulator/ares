struct Nintendo64 : Emulator {
  Nintendo64();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;

  shared_pointer<mia::Pak> gamepad;
};

Nintendo64::Nintendo64() {
  manufacturer = "Nintendo";
  name = "Nintendo 64";
}

auto Nintendo64::load() -> bool {
  game = mia::Medium::create("Nintendo 64");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  system = mia::System::create("Nintendo 64");
  if(!system->load()) return false;

  ares::Nintendo64::option("Quality", settings.video.quality);
  ares::Nintendo64::option("Supersampling", settings.video.supersampling);

  auto region = Emulator::region();
  if(!ares::Nintendo64::load(root, {"[Nintendo] Nintendo 64 (", region, ")"})) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    auto peripheral = port->allocate("Gamepad");
    port->connect();
    if(auto port = peripheral->find<ares::Node::Port>("Pak")) {
      if(!game->pak->read("save.ram")
      && !game->pak->read("save.eeprom")
      && !game->pak->read("save.flash")
      ) {
        gamepad = mia::Pak::create("Nintendo 64");
        gamepad->pak->append("save.pak", 32_KiB);
        gamepad->load("save.pak", ".pak", game->location);
        port->allocate("Controller Pak");
        port->connect();
      } else {
        gamepad.reset();
        port->allocate("Rumble Pak");
        port->connect();
      }
    }
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 2")) {
    auto peripheral = port->allocate("Gamepad");
    port->connect();
    if(auto port = peripheral->find<ares::Node::Port>("Pak")) {
      port->allocate("Rumble Pak");
      port->connect();
    }
  }

  return true;
}

auto Nintendo64::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  if(gamepad) gamepad->save("save.pak", ".pak", game->location);
  return true;
}

auto Nintendo64::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Nintendo 64") return system->pak;
  if(node->name() == "Nintendo 64 Cartridge") return game->pak;
  if(node->name() == "Gamepad") return gamepad->pak;
  return {};
}

auto Nintendo64::input(ares::Node::Input::Input node) -> void {
  auto parent = ares::Node::parent(node);
  if(!parent) return;

  auto port = ares::Node::parent(parent);
  if(!port) return;

  maybe<u32> index;
  if(port->name() == "Controller Port 1") index = 0;
  if(port->name() == "Controller Port 2") index = 1;
  if(!index) return;

  auto name = node->name();
  maybe<InputMapping&> mapping;
  if(name == "X-Axis" ) mapping = virtualPads[*index].lx;
  if(name == "Y-Axis" ) mapping = virtualPads[*index].ly;
  if(name == "Up"     ) mapping = virtualPads[*index].up;
  if(name == "Down"   ) mapping = virtualPads[*index].down;
  if(name == "Left"   ) mapping = virtualPads[*index].left;
  if(name == "Right"  ) mapping = virtualPads[*index].right;
  if(name == "B"      ) mapping = virtualPads[*index].a;
  if(name == "A"      ) mapping = virtualPads[*index].b;
  if(name == "C-Up"   ) mapping = virtualPads[*index].ry;
  if(name == "C-Down" ) mapping = virtualPads[*index].ry;
  if(name == "C-Left" ) mapping = virtualPads[*index].rx;
  if(name == "C-Right") mapping = virtualPads[*index].rx;
  if(name == "L"      ) mapping = virtualPads[*index].l1;
  if(name == "R"      ) mapping = virtualPads[*index].r1;
  if(name == "Z"      ) mapping = virtualPads[*index].z;
  if(name == "Start"  ) mapping = virtualPads[*index].start;
  if(name == "Rumble" ) mapping = virtualPads[*index].rumble;

  if(mapping) {
    auto value = mapping->value();
    if(auto axis = node->cast<ares::Node::Input::Axis>()) {
      axis->setValue(value);
    }
    if(auto button = node->cast<ares::Node::Input::Button>()) {
      if(name == "C-Up"   || name == "C-Left" ) return button->setValue(value < -16384);
      if(name == "C-Down" || name == "C-Right") return button->setValue(value > +16384);
      button->setValue(value);
    }
    if(auto rumble = node->cast<ares::Node::Input::Rumble>()) {
      if(auto target = dynamic_cast<InputRumble*>(mapping.data())) {
        target->rumble(rumble->enable());
      }
    }
  }
}
