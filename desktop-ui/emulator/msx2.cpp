struct MSX2 : MSX {
  MSX2();
  auto load() -> LoadResult override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;
};

MSX2::MSX2() {
  manufacturer = "Microsoft";
  name = "MSX2";

  // TODO: Support other region bios versions
  firmware = {};
  firmware.append({"MAIN", "Japan", "0c672d86ead61a97f49a583b88b7c1905da120645cd44f0c9f2baf4f4631e0b1"});
  firmware.append({"SUB", "Japan", "6c6f421a10c428d960b7ecc990f99af1c638147f747bddca7b0bf0e2ab738300"});

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

auto MSX2::load() -> LoadResult {
  game = mia::Medium::create("MSX2");
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;
  bool isTape = game->pak->attribute("tape").boolean();

  system = mia::System::create("MSX2");
  if(!system->loadMultiple({firmware[0].location, firmware[1].location})) {
    result.result = noFirmware;
    result.firmwareType = firmware[0].type;
    result.firmwareRegion = firmware[0].region;
    result.firmwareSystemName = "MSX2";
    return result;
  }

  auto region = Emulator::region();
  if(!ares::MSX::load(root, {"[Microsoft] MSX2 (", region, ")"})) return otherError;

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

  return successful;
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


