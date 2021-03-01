namespace ares::WonderSwan {
  auto load(Node::System& node, string name) -> bool;
}

struct WonderSwan : Emulator {
  WonderSwan();
  auto load(Menu) -> void override;
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;

  Pak system;
};

struct WonderSwanColor : Emulator {
  WonderSwanColor();
  auto load(Menu) -> void override;
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;

  Pak system;
};

struct PocketChallengeV2 : Emulator {
  PocketChallengeV2();
  auto load(Menu) -> void override;
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;

  Pak system;
};

WonderSwan::WonderSwan() {
  medium = mia::medium("WonderSwan");
  manufacturer = "Bandai";
  name = "WonderSwan";
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
  system.pak = shared_pointer{new vfs::directory};
  system.pak->append("boot.rom", {Resource::WonderSwan::Boot, sizeof Resource::WonderSwan::Boot});

  if(!ares::WonderSwan::load(root, "[Bandai] WonderSwan")) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  return true;
}

auto WonderSwan::save() -> bool {
  root->save();
  return medium->save(game.location, game.pak);
}

auto WonderSwan::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->is<ares::Node::System>()) return system.pak;
  if(node->name() == "WonderSwan") return game.pak;
  return {};
}

auto WonderSwan::input(ares::Node::Input::Input node) -> void {
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

WonderSwanColor::WonderSwanColor() {
  medium = mia::medium("WonderSwan Color");
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
  system.pak = shared_pointer{new vfs::directory};
  system.pak->append("boot.rom", {Resource::WonderSwanColor::Boot, sizeof Resource::WonderSwanColor::Boot});

  if(!ares::WonderSwan::load(root, "[Bandai] WonderSwan Color")) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  return true;
}

auto WonderSwanColor::save() -> bool {
  root->save();
  return medium->save(game.location, game.pak);
}

auto WonderSwanColor::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->is<ares::Node::System>()) return system.pak;
  if(node->name() == "WonderSwan Color") return game.pak;
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

PocketChallengeV2::PocketChallengeV2() {
  medium = mia::medium("Pocket Challenge V2");
  manufacturer = "Benesse";
  name = "Pocket Challenge V2";
}

auto PocketChallengeV2::load(Menu menu) -> void {
  //the Pocket Challenge V2 game library is very small.
  //no titles for the system use portrait (vertical) orientation.
  //as such, neither the ares::WonderSwan core nor lucia provide an orientation setting.
}

auto PocketChallengeV2::load() -> bool {
  system.pak = shared_pointer{new vfs::directory};
  system.pak->append("bios.rom", {Resource::WonderSwan::Boot, sizeof Resource::WonderSwan::Boot});

  if(!ares::WonderSwan::load(root, "[Benesse] Pocket Challenge V2")) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  return true;
}

auto PocketChallengeV2::save() -> bool {
  root->save();
  return medium->save(game.location, game.pak);
}

auto PocketChallengeV2::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->is<ares::Node::System>()) return system.pak;
  if(node->name() == "Pocket Challenge V2") return game.pak;
  return {};
}

auto PocketChallengeV2::input(ares::Node::Input::Input node) -> void {
  auto name = node->name();
  maybe<InputMapping&> mapping;
  if(name == "Up"    ) mapping = virtualPads[0].up;
  if(name == "Down"  ) mapping = virtualPads[0].down;
  if(name == "Left"  ) mapping = virtualPads[0].left;
  if(name == "Right" ) mapping = virtualPads[0].right;
  if(name == "Pass"  ) mapping = virtualPads[0].a;
  if(name == "Circle") mapping = virtualPads[0].b;
  if(name == "Clear" ) mapping = virtualPads[0].y;
  if(name == "View"  ) mapping = virtualPads[0].start;
  if(name == "Escape") mapping = virtualPads[0].select;

  if(mapping) {
    auto value = mapping->value();
    if(auto button = node->cast<ares::Node::Input::Button>()) {
      button->setValue(value);
    }
  }
}
