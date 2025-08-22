struct WonderSwanColor : Emulator {
  WonderSwanColor();
  auto load(Menu) -> void override;
  auto load() -> LoadResult override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
};

WonderSwanColor::WonderSwanColor() {
  manufacturer = "Bandai";
  name = "WonderSwan Color";

  { InputPort port{"WonderSwan Color"};

  { InputDevice device{"Controls"};
    device.digital("Y1",    virtualPorts[0].pad.l_bumper);
    device.digital("Y2",    virtualPorts[0].pad.l_trigger);
    device.digital("Y3",    virtualPorts[0].pad.r_bumper);
    device.digital("Y4",    virtualPorts[0].pad.r_trigger);
    device.digital("X1",    virtualPorts[0].pad.up);
    device.digital("X2",    virtualPorts[0].pad.right);
    device.digital("X3",    virtualPorts[0].pad.down);
    device.digital("X4",    virtualPorts[0].pad.left);
    device.digital("B",     virtualPorts[0].pad.south);
    device.digital("A",     virtualPorts[0].pad.east);
    device.digital("Start", virtualPorts[0].pad.start);
    port.append(device); }

    ports.append(port);
  }
}

auto WonderSwanColor::load(Menu menu) -> void {
  Menu orientationMenu{&menu};
  orientationMenu.setText("Orientation").setIcon(Icon::Device::Display);
  if(auto orientations = root->find<ares::Node::Setting::String>("PPU/Screen/Orientation")) {
    Group group;
    for(auto& orientation : orientations->readAllowedValues()) {
      MenuRadioItem item{&orientationMenu};
      item.setText(orientation);
      item.onActivate([=] {
        Program::Guard guard;
        if(auto orientations = root->find<ares::Node::Setting::String>("PPU/Screen/Orientation")) {
          orientations->setValue(orientation);
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

auto WonderSwanColor::load() -> LoadResult {
  game = mia::Medium::create("WonderSwan Color");
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;

  system = mia::System::create("WonderSwan Color");
  result = system->load();
  if(result != successful) return result;

  ares::WonderSwan::option("Pixel Accuracy", settings.video.pixelAccuracy);

  if(!ares::WonderSwan::load(root, "[Bandai] WonderSwan Color")) return otherError;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  return successful;
}

auto WonderSwanColor::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto WonderSwanColor::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "WonderSwan Color") return system->pak;
  if(node->name() == "WonderSwan Color Cartridge") return game->pak;
  return {};
}
