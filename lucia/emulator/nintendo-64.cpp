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

  for(auto id : range(2)) {
    InputPort port{string{"Controller Port ", 1 + id}};

    InputDevice device{"Gamepad"};
    device.analog("L-Up",    virtualPads[id].lup);
    device.analog("L-Down",  virtualPads[id].ldown);
    device.analog("L-Left",  virtualPads[id].lleft);
    device.analog("L-Right", virtualPads[id].lright);
    device.button("Up",      virtualPads[id].up);
    device.button("Down",    virtualPads[id].down);
    device.button("Left",    virtualPads[id].left);
    device.button("Right",   virtualPads[id].right);
    device.button("B",       virtualPads[id].a);
    device.button("A",       virtualPads[id].b);
    device.button("C-Up",    virtualPads[id].rup);
    device.button("C-Down",  virtualPads[id].rdown);
    device.button("C-Left",  virtualPads[id].rleft);
    device.button("C-Right", virtualPads[id].rright);
    device.button("L",       virtualPads[id].l1);
    device.button("R",       virtualPads[id].r1);
    device.button("Z",       virtualPads[id].z);
    device.button("Start",   virtualPads[id].start);
    device.rumble("Rumble",  virtualPads[id].rumble);
    port.append(device);

    ports.append(port);
  }
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

auto Nintendo64::input(ares::Node::Input::Input input) -> void {
  auto device = ares::Node::parent(input);
  if(!device) return;

  auto port = ares::Node::parent(device);
  if(!port) return;

  maybe<u32> id;
  if(port->name() == "Controller Port 1") id = 0;
  if(port->name() == "Controller Port 2") id = 1;
  if(!id) return;

  if(device->name() == "Gamepad") {
    auto name = input->name();
    maybe<InputMapping&> mappings[2];
    if(name == "X-Axis" ) mappings[0] = virtualPads[*id].lleft, mappings[1] = virtualPads[*id].lright;
    if(name == "Y-Axis" ) mappings[0] = virtualPads[*id].lup,   mappings[1] = virtualPads[*id].ldown;
    if(name == "Up"     ) mappings[0] = virtualPads[*id].up;
    if(name == "Down"   ) mappings[0] = virtualPads[*id].down;
    if(name == "Left"   ) mappings[0] = virtualPads[*id].left;
    if(name == "Right"  ) mappings[0] = virtualPads[*id].right;
    if(name == "B"      ) mappings[0] = virtualPads[*id].a;
    if(name == "A"      ) mappings[0] = virtualPads[*id].b;
    if(name == "C-Up"   ) mappings[0] = virtualPads[*id].rup;
    if(name == "C-Down" ) mappings[0] = virtualPads[*id].rdown;
    if(name == "C-Left" ) mappings[0] = virtualPads[*id].rleft;
    if(name == "C-Right") mappings[0] = virtualPads[*id].rright;
    if(name == "L"      ) mappings[0] = virtualPads[*id].l1;
    if(name == "R"      ) mappings[0] = virtualPads[*id].r1;
    if(name == "Z"      ) mappings[0] = virtualPads[*id].z;
    if(name == "Start"  ) mappings[0] = virtualPads[*id].start;
    if(name == "Rumble" ) mappings[0] = virtualPads[*id].rumble;

    if(mappings[0]) {
      if(auto axis = input->cast<ares::Node::Input::Axis>()) {
        auto value = mappings[1]->value() - mappings[0]->value();
        axis->setValue(value);
      }
      if(auto button = input->cast<ares::Node::Input::Button>()) {
        auto value = mappings[0]->value();
        if(name.beginsWith("C-")) value = abs(value) > +16384;
        button->setValue(value);
      }
      if(auto rumble = input->cast<ares::Node::Input::Rumble>()) {
        if(auto target = dynamic_cast<InputRumble*>(mappings[0].data())) {
          target->rumble(rumble->enable());
        }
      }
    }
  }
}
