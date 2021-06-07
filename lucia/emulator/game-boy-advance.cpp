struct GameBoyAdvance : Emulator {
  GameBoyAdvance();
  auto load(Menu) -> void override;
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;
};

GameBoyAdvance::GameBoyAdvance() {
  manufacturer = "Nintendo";
  name = "Game Boy Advance";

  firmware.append({"BIOS", "World", "fd2547724b505f487e6dcb29ec2ecff3af35a841a77ab2e85fd87350abd36570"});

  { InputPort port{string{"Hardware"}};

    InputDevice device{"Controls"};
    device.button("Up",     virtualPads[0].up);
    device.button("Down",   virtualPads[0].down);
    device.button("Left",   virtualPads[0].left);
    device.button("Right",  virtualPads[0].right);
    device.button("B",      virtualPads[0].a);
    device.button("A",      virtualPads[0].b);
    device.button("L",      virtualPads[0].l1);
    device.button("R",      virtualPads[0].r1);
    device.button("Select", virtualPads[0].select);
    device.button("Start",  virtualPads[0].start);
    device.rumble("Rumble", virtualPads[0].rumble);
    port.append(device);

    ports.append(port);
  }
}

auto GameBoyAdvance::load(Menu menu) -> void {
  Menu orientationMenu{&menu};
  orientationMenu.setText("Orientation").setIcon(Icon::Device::Display);
  if(auto orientations = root->find<ares::Node::Setting::String>("PPU/Screen/Orientation")) {
    Group group;
    for(auto& orientation : orientations->readAllowedValues()) {
      MenuRadioItem item{&orientationMenu};
      item.setText(orientation);
      item.onActivate([=] {
        if(auto orientations = root->find<ares::Node::Setting::String>("PPU/Screen/Orientation")) {
          orientations->setValue(orientation);
        }
      });
      group.append(item);
    }
  }
}

auto GameBoyAdvance::load() -> bool {
  game = mia::Medium::create("Game Boy Advance");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  system = mia::System::create("Game Boy Advance");
  if(!system->load(firmware[0].location)) return errorFirmware(firmware[0]), false;

  if(!ares::GameBoyAdvance::load(root, "[Nintendo] Game Boy Player")) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  return true;
}

auto GameBoyAdvance::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto GameBoyAdvance::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Game Boy Player") return system->pak;
  if(node->name() == "Game Boy Advance Cartridge") return game->pak;
  return {};
}

auto GameBoyAdvance::input(ares::Node::Input::Input input) -> void {
  auto name = input->name();
  maybe<InputMapping&> mapping;
  if(name == "Up"    ) mapping = virtualPads[0].up;
  if(name == "Down"  ) mapping = virtualPads[0].down;
  if(name == "Left"  ) mapping = virtualPads[0].left;
  if(name == "Right" ) mapping = virtualPads[0].right;
  if(name == "B"     ) mapping = virtualPads[0].a;
  if(name == "A"     ) mapping = virtualPads[0].b;
  if(name == "L"     ) mapping = virtualPads[0].l1;
  if(name == "R"     ) mapping = virtualPads[0].r1;
  if(name == "Select") mapping = virtualPads[0].select;
  if(name == "Start" ) mapping = virtualPads[0].start;
  //Game Boy Player
  if(name == "Rumble") mapping = virtualPads[0].rumble;

  if(mapping) {
    auto value = mapping->value();
    if(auto button = input->cast<ares::Node::Input::Button>()) {
      button->setValue(value);
    }
    if(auto rumble = input->cast<ares::Node::Input::Rumble>()) {
      if(auto target = dynamic_cast<InputRumble*>(mapping.data())) {
        target->rumble(rumble->enable());
      }
    }
  }
}
