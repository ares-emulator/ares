struct GameBoy : Emulator {
  GameBoy();
  auto load() -> LoadResult override;
  auto load(Menu) -> void override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
};

GameBoy::GameBoy() {
  manufacturer = "Nintendo";
  name = "Game Boy";

  { InputPort port{"Game Boy"};

  { InputDevice device{"Controls"};
    device.digital("Up",      virtualPorts[0].pad.up);
    device.digital("Down",    virtualPorts[0].pad.down);
    device.digital("Left",    virtualPorts[0].pad.left);
    device.digital("Right",   virtualPorts[0].pad.right);
    device.digital("B",       virtualPorts[0].pad.south);
    device.digital("A",       virtualPorts[0].pad.east);
    device.digital("Select",  virtualPorts[0].pad.select);
    device.digital("Start",   virtualPorts[0].pad.start);
    device.rumble ("Rumble",  virtualPorts[0].pad.rumble);
    port.append(device); }

    ports.append(port);
  }
}

auto GameBoy::load() -> LoadResult {
  game = mia::Medium::create("Game Boy");
  string location = Emulator::load(game, configuration.game);
  if(!location) return LoadResult(noFileSelected);
  LoadResult result = game->load(location);
  if(result != LoadResult(successful)) return result;

  system = mia::System::create("Game Boy");
  result = system->load();
  if(result != LoadResult(successful)) return result;

  if(!ares::GameBoy::load(root, "[Nintendo] Game Boy")) return LoadResult(otherError);

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto fastBoot = root->find<ares::Node::Setting::Boolean>("Fast Boot")) {
    fastBoot->setValue(settings.boot.fast);
  }

  return LoadResult(successful);
}

auto GameBoy::load(Menu menu) -> void {
    Menu colorEmulationMenu{&menu};
    colorEmulationMenu.setText("Color Emulation").setIcon(Icon::Device::Display);
    if(auto options = root->find<ares::Node::Setting::String>("PPU/Screen/Color Emulation")) {
      Group group;
      for(auto& setting : options->readAllowedValues()) {
        MenuRadioItem item{&colorEmulationMenu};
        item.setText(setting);
        item.onActivate([=] {
          if(auto settings = root->find<ares::Node::Setting::String>("PPU/Screen/Color Emulation")) {
            settings->setValue(setting);
          }
        });
        group.append(item);
      }
    }

    if(auto headphones = root->find<ares::Node::Setting::Boolean>("Headphones")) {
        MenuCheckItem headphoneItem{&menu};
        headphoneItem.setText("Headphones").setChecked(headphones->value()).onToggle([=] {
            if(auto headphones = root->find<ares::Node::Setting::Boolean>("Headphones")) {
                headphones->setValue(headphoneItem.checked());
            }
        });
    }
}

auto GameBoy::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto GameBoy::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Game Boy") return system->pak;
  if(node->name() == "Game Boy Cartridge") return game->pak;
  return {};
}
