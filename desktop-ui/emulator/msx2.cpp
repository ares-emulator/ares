struct MSX2 : MSX {
  MSX2();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;
};

MSX2::MSX2() {
  manufacturer = "Microsoft";
  name = "MSX2";

  // TODO: Support other region bios versions
  firmware = {};
  firmware.append({"MAIN", "Japan"});
  firmware.append({"SUB", "Japan"});

  for(auto id : range(2)) {
    InputPort port{string{"Controller Port ", 1 + id}};

  { InputDevice device{"Gamepad"};
    device.digital("Up",    virtualPorts[id].pad.up);
    device.digital("Down",  virtualPorts[id].pad.down);
    device.digital("Left",  virtualPorts[id].pad.left);
    device.digital("Right", virtualPorts[id].pad.right);
    device.digital("A",     virtualPorts[id].pad.east);
    device.digital("B",     virtualPorts[id].pad.south);
    port.append(device); }

    ports.append(port);
  }
}

auto MSX2::load() -> bool {
  game = mia::Medium::create("MSX2");
  if(!game->load(Emulator::load(game, configuration.game))) return false;
  bool isTape = game->pak->attribute("tape").boolean();

  system = mia::System::create("MSX2");
  if(!system->loadMultiple({firmware[0].location, firmware[1].location})) {
    return errorFirmware(firmware[0]), false;
  }

  auto region = Emulator::region();
  if(!ares::MSX::load(root, {"[Microsoft] MSX2 (", region, ")"})) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    if(!isTape) port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Tape Deck/Tray")) {
    port->allocate();
    if(isTape) port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    port->allocate("Gamepad");
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 2")) {
    port->allocate("Gamepad");
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Keyboard")) {
    port->allocate("Japanese");
    port->connect();
  }

  return true;
}

auto MSX2::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto MSX2::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "MSX2") return system->pak;
  if(node->name() == "MSX2 Cartridge") return game->pak;
  if(node->name() == "MSX Tape") return game->pak;
  return {};
}

auto MSX2::input(ares::Node::Input::Input input) -> void {
  MSX::input(input);
}


