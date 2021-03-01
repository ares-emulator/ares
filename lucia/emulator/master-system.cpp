namespace ares::MasterSystem {
  auto load(Node::System& node, string name) -> bool;
}

struct MasterSystem : Emulator {
  MasterSystem();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;
};

struct GameGear : Emulator {
  GameGear();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;
};

MasterSystem::MasterSystem() {
  medium = mia::medium("Master System");
  manufacturer = "Sega";
  name = "Master System";
}

auto MasterSystem::load() -> bool {
  auto region = Emulator::region();
  if(!ares::MasterSystem::load(root, {"[Sega] Master System (", region, ")"})) return false;

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

  if(auto port = root->find<ares::Node::Port>("Expansion Port")) {
    port->allocate("FM Sound Unit");
    port->connect();
  }

  return true;
}

auto MasterSystem::save() -> bool {
  root->save();
  return medium->save(game.location, game.pak);
}

auto MasterSystem::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Master System") return game.pak;
  return {};
}

auto MasterSystem::input(ares::Node::Input::Input node) -> void {
  auto parent = ares::Node::parent(node);
  if(!parent) return;

  auto port = ares::Node::parent(parent);
  if(!port) return;

  maybe<u32> index;
  if(port->name() == "Controller Port 1") index = 0;
  if(port->name() == "Controller Port 2") index = 1;
  if(!index) return;

  if(parent->name() == "Gamepad") {
    auto name = node->name();
    maybe<InputMapping&> mapping;
    if(name == "Pause") mapping = virtualPads[*index].start;
    if(name == "Reset") mapping = nothing;
    if(name == "Up"   ) mapping = virtualPads[*index].up;
    if(name == "Down" ) mapping = virtualPads[*index].down;
    if(name == "Left" ) mapping = virtualPads[*index].left;
    if(name == "Right") mapping = virtualPads[*index].right;
    if(name == "1"    ) mapping = virtualPads[*index].a;
    if(name == "2"    ) mapping = virtualPads[*index].b;

    if(mapping) {
      auto value = mapping->value();
      if(auto button = node->cast<ares::Node::Input::Button>()) {
        button->setValue(value);
      }
    }
  }
}

GameGear::GameGear() {
  medium = mia::medium("Game Gear");
  manufacturer = "Sega";
  name = "Game Gear";
}

auto GameGear::load() -> bool {
  if(!ares::MasterSystem::load(root, "[Sega] Game Gear")) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  return true;
}

auto GameGear::save() -> bool {
  root->save();
  return medium->save(game.location, game.pak);
}

auto GameGear::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Game Gear") return game.pak;
  return {};
}

auto GameGear::input(ares::Node::Input::Input node) -> void {
  auto name = node->name();
  maybe<InputMapping&> mapping;
  if(name == "Up"   ) mapping = virtualPads[0].up;
  if(name == "Down" ) mapping = virtualPads[0].down;
  if(name == "Left" ) mapping = virtualPads[0].left;
  if(name == "Right") mapping = virtualPads[0].right;
  if(name == "1"    ) mapping = virtualPads[0].a;
  if(name == "2"    ) mapping = virtualPads[0].b;
  if(name == "Start") mapping = virtualPads[0].start;

  if(mapping) {
    auto value = mapping->value();
    if(auto button = node->cast<ares::Node::Input::Button>()) {
      button->setValue(value);
    }
  }
}
