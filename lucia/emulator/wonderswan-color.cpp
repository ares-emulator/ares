struct WonderSwanColor : Emulator {
  WonderSwanColor();
  auto load(Menu) -> void override;
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;
};

WonderSwanColor::WonderSwanColor() {
  manufacturer = "Bandai";
  name = "WonderSwan Color";
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
        if(auto orientations = root->find<ares::Node::Setting::String>("PPU/Screen/Orientation")) {
          orientations->setValue(orientation);
        }
      });
      group.append(item);
    }
  }
}

auto WonderSwanColor::load() -> bool {
  game = mia::Medium::create("WonderSwan Color");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  system = mia::System::create("WonderSwan Color");
  if(!system->load()) return false;

  if(!ares::WonderSwan::load(root, "[Bandai] WonderSwan Color")) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  return true;
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

auto WonderSwanColor::input(ares::Node::Input::Input node) -> void {
  auto name = node->name();
  maybe<InputMapping&> mapping;
  if(name == "Y1"   ) mapping = virtualPads[0].l1;
  if(name == "Y2"   ) mapping = virtualPads[0].l2;
  if(name == "Y3"   ) mapping = virtualPads[0].r1;
  if(name == "Y4"   ) mapping = virtualPads[0].r2;
  if(name == "X1"   ) mapping = virtualPads[0].up;
  if(name == "X2"   ) mapping = virtualPads[0].right;
  if(name == "X3"   ) mapping = virtualPads[0].down;
  if(name == "X4"   ) mapping = virtualPads[0].left;
  if(name == "B"    ) mapping = virtualPads[0].a;
  if(name == "A"    ) mapping = virtualPads[0].b;
  if(name == "Start") mapping = virtualPads[0].start;

  if(mapping) {
    auto value = mapping->value();
    if(auto button = node->cast<ares::Node::Input::Button>()) {
      button->setValue(value);
    }
  }
}
