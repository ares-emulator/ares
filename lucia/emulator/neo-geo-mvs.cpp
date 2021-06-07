struct NeoGeoMVS : Emulator {
  NeoGeoMVS();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;
};

NeoGeoMVS::NeoGeoMVS() {
  manufacturer = "SNK";
  name = "Neo Geo MVS";

  firmware.append({"BIOS", "World"});

  for(auto id : range(2)) {
    InputPort port{string{"Controller Port ", 1 + id}};

    InputDevice device{"Arcade Stick"};
    device.button("Up",     virtualPads[id].up);
    device.button("Down",   virtualPads[id].down);
    device.button("Left",   virtualPads[id].left);
    device.button("Right",  virtualPads[id].right);
    device.button("A",      virtualPads[id].a);
    device.button("B",      virtualPads[id].b);
    device.button("C",      virtualPads[id].x);
    device.button("D",      virtualPads[id].y);
    device.button("Select", virtualPads[id].select);
    device.button("Start",  virtualPads[id].start);
    port.append(device);

    ports.append(port);
  }
}

auto NeoGeoMVS::load() -> bool {
  game = mia::Medium::create("Neo Geo");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  system = mia::System::create("Neo Geo MVS");
  if(!system->load(firmware[0].location)) return errorFirmware(firmware[0]), false;

  if(!ares::NeoGeo::load(root, "[SNK] Neo Geo MVS")) return false;

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

auto NeoGeoMVS::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto NeoGeoMVS::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Neo Geo MVS") return system->pak;
  if(node->name() == "Neo Geo Cartridge") return game->pak;
  return {};
}

auto NeoGeoMVS::input(ares::Node::Input::Input input) -> void {
  auto device = ares::Node::parent(input);
  if(!device) return;

  auto port = ares::Node::parent(device);
  if(!port) return;

  maybe<u32> id;
  if(port->name() == "Controller Port 1") id = 0;
  if(port->name() == "Controller Port 2") id = 1;
  if(!id) return;

  if(device->name() == "Arcade Stick") {
    auto name = input->name();
    maybe<InputMapping&> mapping;
    if(name == "Up"    ) mapping = virtualPads[*id].up;
    if(name == "Down"  ) mapping = virtualPads[*id].down;
    if(name == "Left"  ) mapping = virtualPads[*id].left;
    if(name == "Right" ) mapping = virtualPads[*id].right;
    if(name == "A"     ) mapping = virtualPads[*id].a;
    if(name == "B"     ) mapping = virtualPads[*id].b;
    if(name == "C"     ) mapping = virtualPads[*id].x;
    if(name == "D"     ) mapping = virtualPads[*id].y;
    if(name == "Select") mapping = virtualPads[*id].select;
    if(name == "Start" ) mapping = virtualPads[*id].start;

    if(mapping) {
      auto value = mapping->value();
      if(auto button = input->cast<ares::Node::Input::Button>()) {
        button->setValue(value);
      }
    }
  }
}
