struct WonderSwan : Emulator {
  WonderSwan();
  auto load(Menu) -> void override;
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
};

WonderSwan::WonderSwan() {
  manufacturer = "Bandai";
  name = "WonderSwan";

  { InputPort port{"WonderSwan"};

  { InputDevice device{"Controls"};
    device.digital("Y1",    virtualPorts[0].pad.l1);
    device.digital("Y2",    virtualPorts[0].pad.l2);
    device.digital("Y3",    virtualPorts[0].pad.r1);
    device.digital("Y4",    virtualPorts[0].pad.r2);
    device.digital("X1",    virtualPorts[0].pad.up);
    device.digital("X2",    virtualPorts[0].pad.right);
    device.digital("X3",    virtualPorts[0].pad.down);
    device.digital("X4",    virtualPorts[0].pad.left);
    device.digital("B",     virtualPorts[0].pad.a);
    device.digital("A",     virtualPorts[0].pad.b);
    device.digital("Start", virtualPorts[0].pad.start);
    port.append(device); }

    ports.append(port);
  }
}

auto WonderSwan::load(Menu menu) -> void {
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

auto WonderSwan::load() -> bool {
  game = mia::Medium::create("WonderSwan");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  system = mia::System::create("WonderSwan");
  if(!system->load()) return false;

  if(!ares::WonderSwan::load(root, "[Bandai] WonderSwan")) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  return true;
}

auto WonderSwan::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto WonderSwan::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "WonderSwan") return system->pak;
  if(node->name() == "WonderSwan Cartridge") return game->pak;
  return {};
}
