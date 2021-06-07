struct MSX2 : Emulator {
  MSX2();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;
};

MSX2::MSX2() {
  manufacturer = "Microsoft";
  name = "MSX2";

  for(auto id : range(2)) {
    InputPort port{string{"Controller Port ", 1 + id}};

    InputDevice device{"Gamepad"};
    device.button("Up",    virtualPads[id].up);
    device.button("Down",  virtualPads[id].down);
    device.button("Left",  virtualPads[id].left);
    device.button("Right", virtualPads[id].right);
    device.button("A",     virtualPads[id].a);
    device.button("B",     virtualPads[id].b);
    port.append(device);

    ports.append(port);
  }
}

auto MSX2::load() -> bool {
  game = mia::Medium::create("MSX2");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  system = mia::System::create("MSX2");
  if(!system->load()) return false;

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

  if(auto port = root->find<ares::Node::Port>("Controller Port 2")) {
    port->allocate("Gamepad");
    port->connect();
  }

  return true;
}

auto MSX2::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto MSX2::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "MSX2") return system->pak;
  if(node->name() == "MSX2 Cartridge") return game->pak;
  return {};
}

auto MSX2::input(ares::Node::Input::Input input) -> void {
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
    maybe<InputMapping&> mapping;
    if(name == "Up"   ) mapping = virtualPads[*id].up;
    if(name == "Down" ) mapping = virtualPads[*id].down;
    if(name == "Left" ) mapping = virtualPads[*id].left;
    if(name == "Right") mapping = virtualPads[*id].right;
    if(name == "A"    ) mapping = virtualPads[*id].a;
    if(name == "B"    ) mapping = virtualPads[*id].b;

    if(mapping) {
      auto value = mapping->value();
      if(auto button = input->cast<ares::Node::Input::Button>()) {
        button->setValue(value);
      }
    }
  }
}
