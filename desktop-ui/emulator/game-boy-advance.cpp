struct GameBoyAdvance : Emulator {
  GameBoyAdvance();
  auto load(Menu) -> void override;
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
};

GameBoyAdvance::GameBoyAdvance() {
  manufacturer = "Nintendo";
  name = "Game Boy Advance";

  firmware.append({"BIOS", "World", "fd2547724b505f487e6dcb29ec2ecff3af35a841a77ab2e85fd87350abd36570"});

  { InputPort port{string{"Game Boy Advance"}};

  { InputDevice device{"Controls"};
    device.digital("Up",     virtualPorts[0].pad.up);
    device.digital("Down",   virtualPorts[0].pad.down);
    device.digital("Left",   virtualPorts[0].pad.left);
    device.digital("Right",  virtualPorts[0].pad.right);
    device.digital("B",      virtualPorts[0].pad.a);
    device.digital("A",      virtualPorts[0].pad.b);
    device.digital("L",      virtualPorts[0].pad.l1);
    device.digital("R",      virtualPorts[0].pad.r1);
    device.digital("Select", virtualPorts[0].pad.select);
    device.digital("Start",  virtualPorts[0].pad.start);
    device.rumble ("Rumble", virtualPorts[0].pad.rumble);
    port.append(device); }

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

  if(!ares::GameBoyAdvance::load(root, "[Nintendo] Game Boy Advance")) return false;

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
  if(node->name() == "Game Boy Advance") return system->pak;
  if(node->name() == "Game Boy Advance Cartridge") return game->pak;
  return {};
}
