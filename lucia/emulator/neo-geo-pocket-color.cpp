struct NeoGeoPocketColor : Emulator {
  NeoGeoPocketColor();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
};

NeoGeoPocketColor::NeoGeoPocketColor() {
  manufacturer = "SNK";
  name = "Neo Geo Pocket Color";

  firmware.append({"BIOS", "World", "8fb845a2f71514cec20728e2f0fecfade69444f8d50898b92c2259f1ba63e10d"});

  { InputPort port{"Neo Geo Pocket Color"};

  { InputDevice device{"Controls"};
    device.digital("Up",       virtualPorts[0].pad.up);
    device.digital("Down",     virtualPorts[0].pad.down);
    device.digital("Left",     virtualPorts[0].pad.left);
    device.digital("Right",    virtualPorts[0].pad.right);
    device.digital("A",        virtualPorts[0].pad.a);
    device.digital("B",        virtualPorts[0].pad.b);
    device.digital("Option",   virtualPorts[0].pad.start);
    device.digital("Power",    virtualPorts[0].pad.lt);
    device.digital("Debugger", virtualPorts[0].pad.rt);
    port.append(device); }

    ports.append(port);
  }
}

auto NeoGeoPocketColor::load() -> bool {
  game = mia::Medium::create("Neo Geo Pocket Color");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  system = mia::System::create("Neo Geo Pocket Color");
  if(!system->load(firmware[0].location)) return errorFirmware(firmware[0]), false;

  if(!ares::NeoGeoPocket::load(root, "[SNK] Neo Geo Pocket Color")) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto fastBoot = root->find<ares::Node::Setting::Boolean>("Fast Boot")) {
    fastBoot->setValue(settings.boot.fast);
  }

  return true;
}

auto NeoGeoPocketColor::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto NeoGeoPocketColor::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Neo Geo Pocket Color") return system->pak;
  if(node->name() == "Neo Geo Pocket Color Cartridge") return game->pak;
  return {};
}
