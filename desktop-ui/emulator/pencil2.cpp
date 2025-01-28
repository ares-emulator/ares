struct Pencil2 : Emulator {
  Pencil2();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
};

Pencil2::Pencil2() {
  manufacturer = "Hanimex";
  name = "Pencil 2";

  firmware.append({"BIOS", "World", "b03f77561ee75d4ce17752b9df8522389bfe4a0b1ec608cddfd4ea3af193580d"});

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

auto Pencil2::load() -> bool {
  game = mia::Medium::create("Pencil 2");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  system = mia::System::create("Pencil 2");
  if(!system->load(firmware[0].location)) return errorFirmware(firmware[0]), false;

  auto region = Emulator::region();
  if(!ares::Pencil2::load(root, {"[Hanimex] Pencil 2 (", region, ")"})) return false;

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

  return true;
}

auto Pencil2::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto Pencil2::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Pencil 2") return system->pak;
  if(node->name() == "Pencil 2 Cartridge") return game->pak;
  return {};
}
