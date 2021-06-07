struct SuperGrafx : Emulator {
  SuperGrafx();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;
};

SuperGrafx::SuperGrafx() {
  manufacturer = "NEC";
  name = "SuperGrafx";

  { InputPort port{"Controller Port"};

    InputDevice device{"Gamepad"};
    device.button("Up",     virtualPads[0].up);
    device.button("Down",   virtualPads[0].down);
    device.button("Left",   virtualPads[0].left);
    device.button("Right",  virtualPads[0].right);
    device.button("II",     virtualPads[0].a);
    device.button("I",      virtualPads[0].b);
    device.button("Select", virtualPads[0].select);
    device.button("Run",    virtualPads[0].start);
    port.append(device);

    ports.append(port);
  }
}

auto SuperGrafx::load() -> bool {
  game = mia::Medium::create("SuperGrafx");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  system = mia::System::create("SuperGrafx");
  if(!system->load()) return false;

  if(!ares::PCEngine::load(root, "[NEC] SuperGrafx (NTSC-J)")) return false;

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

auto SuperGrafx::save() -> bool {
  root->save();
  system->save(game->location);
  game->save(game->location);
  return true;
}

auto SuperGrafx::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "SuperGrafx") return system->pak;
  if(node->name() == "SuperGrafx Card") return game->pak;
  return {};
}

auto SuperGrafx::input(ares::Node::Input::Input input) -> void {
  auto name = input->name();
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
    if(auto button = input->cast<ares::Node::Input::Button>()) {
      button->setValue(value);
    }
  }
}
