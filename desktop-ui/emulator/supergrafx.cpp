struct SuperGrafx : Emulator {
  SuperGrafx();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
};

SuperGrafx::SuperGrafx() {
  manufacturer = "NEC";
  name = "SuperGrafx";

  for(auto id : range(5)) {
   InputPort port{string{"Controller Port ", 1 + id}};

  { InputDevice device{"Gamepad"};
    device.digital("Up",     virtualPorts[id].pad.up);
    device.digital("Down",   virtualPorts[id].pad.down);
    device.digital("Left",   virtualPorts[id].pad.left);
    device.digital("Right",  virtualPorts[id].pad.right);
    device.digital("II",     virtualPorts[id].pad.a);
    device.digital("I",      virtualPorts[id].pad.b);
    device.digital("Select", virtualPorts[id].pad.select);
    device.digital("Run",    virtualPorts[id].pad.start);
    port.append(device); }

    ports.append(port);
  }
}

auto SuperGrafx::load() -> bool {
  game = mia::Medium::create("SuperGrafx");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  system = mia::System::create("SuperGrafx");
  if(!system->load()) return false;

  if(!ares::PCEngine::load(root, "[NEC] SuperGrafx (NTSC-J)")) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port")) {
    port->allocate("Multitap");
    port->connect();
  }

  for(auto id : range(5)) {
    if(auto port = root->scan<ares::Node::Port>(string{"Controller Port ", 1 + id})) {
      port->allocate("Gamepad");
      port->connect();
    }
  }

  return true;
}

auto SuperGrafx::save() -> bool {
  root->save();
  system->save(game->location);
  game->save(game->location);
  return true;
}

auto SuperGrafx::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "SuperGrafx") return system->pak;
  if(node->name() == "SuperGrafx Card") return game->pak;
  return {};
}
