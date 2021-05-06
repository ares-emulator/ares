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
  maybe<InputMapping&> mappings[2];
  if(name == "X-Axis" ) mappings[0] = virtualPads[*index].lleft, mappings[1] = virtualPads[*index].lright;
  if(name == "Y-Axis" ) mappings[0] = virtualPads[*index].lup,   mappings[1] = virtualPads[*index].ldown;
  if(name == "Up"     ) mappings[0] = virtualPads[*index].up;
  if(name == "Down"   ) mappings[0] = virtualPads[*index].down;
  if(name == "Left"   ) mappings[0] = virtualPads[*index].left;
  if(name == "Right"  ) mappings[0] = virtualPads[*index].right;
  if(name == "B"      ) mappings[0] = virtualPads[*index].a;
  if(name == "A"      ) mappings[0] = virtualPads[*index].b;
  if(name == "C-Up"   ) mappings[0] = virtualPads[*index].rup;
  if(name == "C-Down" ) mappings[0] = virtualPads[*index].rdown;
  if(name == "C-Left" ) mappings[0] = virtualPads[*index].rleft;
  if(name == "C-Right") mappings[0] = virtualPads[*index].rright;
  if(name == "L"      ) mappings[0] = virtualPads[*index].l1;
  if(name == "R"      ) mappings[0] = virtualPads[*index].r1;
  if(name == "Z"      ) mappings[0] = virtualPads[*index].z;
  if(name == "Start"  ) mappings[0] = virtualPads[*index].start;
  if(name == "Rumble" ) mappings[0] = virtualPads[*index].rumble;

  if(mappings[0]) {
    if(auto axis = node->cast<ares::Node::Input::Axis>()) {
      auto value = mappings[1]->value() - mappings[0]->value();
      axis->setValue(value);
    }
    if(auto button = node->cast<ares::Node::Input::Button>()) {
      auto value = mappings[0]->value();
      if(name.beginsWith("C-")) value = abs(value) > +16384;
      button->setValue(value);
    }
    if(auto rumble = node->cast<ares::Node::Input::Rumble>()) {
      if(auto target = dynamic_cast<InputRumble*>(mappings[0].data())) {
        target->rumble(rumble->enable());
      }
    }
  }
}
