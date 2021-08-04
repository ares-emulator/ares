struct GameBoy : Emulator {
  GameBoy();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
};

GameBoy::GameBoy() {
  manufacturer = "Nintendo";
  name = "Game Boy";

  { InputPort port{"Game Boy"};

  { InputDevice device{"Controls"};
    device.digital("Up",      virtualPorts[0].pad.up);
    device.digital("Down",    virtualPorts[0].pad.down);
    device.digital("Left",    virtualPorts[0].pad.left);
    device.digital("Right",   virtualPorts[0].pad.right);
    device.digital("B",       virtualPorts[0].pad.a);
    device.digital("A",       virtualPorts[0].pad.b);
    device.digital("Select",  virtualPorts[0].pad.select);
    device.digital("Start",   virtualPorts[0].pad.start);
    device.analog ("A-Up",    virtualPorts[0].pad.lup);
    device.analog ("A-Down",  virtualPorts[0].pad.ldown);
    device.analog ("A-Left",  virtualPorts[0].pad.lleft);
    device.analog ("A-Right", virtualPorts[0].pad.lright);
    device.rumble ("Rumble",  virtualPorts[0].pad.rumble);
    port.append(device); }

    ports.append(port);
  }
}

auto GameBoy::load() -> bool {
  game = mia::Medium::create("Game Boy");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  system = mia::System::create("Game Boy");
  if(!system->load()) return false;

  if(!ares::GameBoy::load(root, "[Nintendo] Game Boy")) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto fastBoot = root->find<ares::Node::Setting::Boolean>("Fast Boot")) {
    fastBoot->setValue(settings.boot.fast);
  }

  return true;
}

auto GameBoy::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto GameBoy::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Game Boy") return system->pak;
  if(node->name() == "Game Boy Cartridge") return game->pak;
  return {};
}
