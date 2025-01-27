struct ColecoVision : Emulator {
  ColecoVision();
  auto load() -> LoadResult override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
};

ColecoVision::ColecoVision() {
  manufacturer = "Coleco";
  name = "ColecoVision";

  firmware.append({"BIOS", "World", "990bf1956f10207d8781b619eb74f89b00d921c8d45c95c334c16c8cceca09ad"});

  for(auto id : range(2)) {
    InputPort port{string{"Controller Port ", 1 + id}};

  { InputDevice device{"Gamepad"};
    device.digital("Up",    virtualPorts[id].pad.up);
    device.digital("Down",  virtualPorts[id].pad.down);
    device.digital("Left",  virtualPorts[id].pad.left);
    device.digital("Right", virtualPorts[id].pad.right);
    device.digital("L",     virtualPorts[id].pad.south);
    device.digital("R",     virtualPorts[id].pad.east);
    device.digital("1",     virtualPorts[id].pad.west);
    device.digital("2",     virtualPorts[id].pad.north);
    device.digital("3",     virtualPorts[id].pad.l_bumper);
    device.digital("4",     virtualPorts[id].pad.l_trigger);
    device.digital("5",     virtualPorts[id].pad.r_bumper);
    device.digital("6",     virtualPorts[id].pad.r_trigger);
    device.digital("7",     virtualPorts[id].pad.lstick_click);
    device.digital("8",     virtualPorts[id].pad.rstick_click);
    device.digital("9",     virtualPorts[id].pad.rstick_down);
    device.digital("0",     virtualPorts[id].pad.rstick_right);
    device.digital("*",     virtualPorts[id].pad.select);
    device.digital("#",     virtualPorts[id].pad.start);
    port.append(device); }

    ports.append(port);
  }
}

auto ColecoVision::load() -> LoadResult {
  game = mia::Medium::create("ColecoVision");
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;

  system = mia::System::create("ColecoVision");
  if(system->load(firmware[0].location) != successful) {
    result.firmwareSystemName = "ColecoVision";
    result.firmwareType = firmware[0].type;
    result.firmwareRegion = firmware[0].region;
    result.result = noFirmware;
    return result;
  }

  auto region = Emulator::region();
  if(!ares::ColecoVision::load(root, {"[Coleco] ColecoVision (", region, ")"})) return otherError;

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

  return successful;
}

auto ColecoVision::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto ColecoVision::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "ColecoVision") return system->pak;
  if(node->name() == "ColecoVision Cartridge") return game->pak;
  return {};
}
