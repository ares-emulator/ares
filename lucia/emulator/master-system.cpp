struct MasterSystem : Emulator {
  MasterSystem();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;

  u32 regionID = 0;
};

MasterSystem::MasterSystem() {
  manufacturer = "Sega";
  name = "Master System";

  firmware.append({"BIOS", "US"});      //NTSC-U
  firmware.append({"BIOS", "Japan"});   //NTSC-J
  firmware.append({"BIOS", "Europe"});  //PAL

  { InputPort port{"Hardware"};

    InputDevice device{"Controls"};
    device.button("Pause", virtualPads[0].start);
    device.button("Reset", virtualPads[0].rt);
    port.append(device);

    ports.append(port);
  }

  for(auto id : range(2)) {
    InputPort port{string{"Controller Port ", 1 + id}};

    InputDevice device{"Gamepad"};
    device.button("Up",    virtualPads[id].up);
    device.button("Down",  virtualPads[id].down);
    device.button("Left",  virtualPads[id].left);
    device.button("Right", virtualPads[id].right);
    device.button("1",     virtualPads[id].a);
    device.button("2",     virtualPads[id].b);
    port.append(device);

    ports.append(port);
  }
}

auto MasterSystem::load() -> bool {
  game = mia::Medium::create("Master System");
  game->load(Emulator::load(game, configuration.game));

  auto region = Emulator::region();
  //if statements below are ordered by lowest to highest priority
  if(region == "PAL"   ) regionID = 2;
  if(region == "NTSC-J") regionID = 1;
  if(region == "NTSC-U") regionID = 0;

  system = mia::System::create("Master System");
  if(!system->load(firmware[regionID].location)) return false;
  if(!game->pak && !system->pak->read("bios.rom")) return false;

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
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto MasterSystem::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Master System") return system->pak;
  if(node->name() == "Master System Cartridge") return game->pak;
  return {};
}

auto MasterSystem::input(ares::Node::Input::Input input) -> void {
  auto device = ares::Node::parent(input);
  if(!device) return;

  if(device->name() == "Controls") {
    auto name = input->name();
    maybe<InputMapping&> mapping;
    if(name == "Pause") mapping = virtualPads[0].start;
    if(name == "Reset") mapping = virtualPads[0].rt;

    if(mapping) {
      auto value = mapping->value();
      if(auto button = input->cast<ares::Node::Input::Button>()) {
        button->setValue(value);
      }
    }
    return;
  }

  auto port = ares::Node::parent(device);
  if(!port) return;

  maybe<u32> id;
  if(port->name() == "Controller Port 1") id = 0;
  if(port->name() == "Controller Port 2") id = 1;
  if(!id) return;

  if(device->name() == "Gamepad") {
    auto name = input->name();
    maybe<InputMapping&> mapping;
    if(name == "Up"   ) mapping = virtualPads[*id].up;
    if(name == "Down" ) mapping = virtualPads[*id].down;
    if(name == "Left" ) mapping = virtualPads[*id].left;
    if(name == "Right") mapping = virtualPads[*id].right;
    if(name == "1"    ) mapping = virtualPads[*id].a;
    if(name == "2"    ) mapping = virtualPads[*id].b;

    if(mapping) {
      auto value = mapping->value();
      if(auto button = input->cast<ares::Node::Input::Button>()) {
        button->setValue(value);
      }
    }
  }
}
