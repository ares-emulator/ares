struct NeoGeoAES : Emulator {
  NeoGeoAES();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;
};

NeoGeoAES::NeoGeoAES() {
  manufacturer = "SNK";
  name = "Neo Geo AES";

  firmware.append({"BIOS", "World"});
}

auto NeoGeoAES::load() -> bool {
  game = mia::Medium::create("Neo Geo");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  system = mia::System::create("Neo Geo AES");
  if(!system->load(firmware[0].location)) return errorFirmware(firmware[0]), false;

  if(!ares::NeoGeo::load(root, "[SNK] Neo Geo AES")) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    port->allocate("Arcade Stick");
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 2")) {
    port->allocate("Arcade Stick");
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Memory Card Slot")) {
    port->allocate("Memory Card");
    port->connect();
  }

  return true;
}

auto NeoGeoAES::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto NeoGeoAES::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Neo Geo AES") return system->pak;
  if(node->name() == "Neo Geo Cartridge") return game->pak;
  return {};
}

auto NeoGeoAES::input(ares::Node::Input::Input node) -> void {
  auto parent = ares::Node::parent(node);
  if(!parent) return;

  auto port = ares::Node::parent(parent);
  if(!port) return;

  maybe<u32> index;
  if(port->name() == "Controller Port 1") index = 0;
  if(port->name() == "Controller Port 2") index = 1;
  if(!index) return;

  if(parent->name() == "Arcade Stick") {
    auto name = node->name();
    maybe<InputMapping&> mapping;
    if(name == "Up"    ) mapping = virtualPads[*index].up;
    if(name == "Down"  ) mapping = virtualPads[*index].down;
    if(name == "Left"  ) mapping = virtualPads[*index].left;
    if(name == "Right" ) mapping = virtualPads[*index].right;
    if(name == "A"     ) mapping = virtualPads[*index].a;
    if(name == "B"     ) mapping = virtualPads[*index].b;
    if(name == "C"     ) mapping = virtualPads[*index].x;
    if(name == "D"     ) mapping = virtualPads[*index].y;
    if(name == "Select") mapping = virtualPads[*index].select;
    if(name == "Start" ) mapping = virtualPads[*index].start;

    if(mapping) {
      auto value = mapping->value();
      if(auto button = node->cast<ares::Node::Input::Button>()) {
        button->setValue(value);
      }
    }
  }
}
