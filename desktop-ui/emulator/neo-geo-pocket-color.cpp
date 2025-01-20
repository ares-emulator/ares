struct NeoGeoPocketColor : Emulator {
  NeoGeoPocketColor();
  auto load() -> LoadResult override;
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
    device.digital("A",        virtualPorts[0].pad.south);
    device.digital("B",        virtualPorts[0].pad.east);
    device.digital("Option",   virtualPorts[0].pad.start);
    device.digital("Power",    virtualPorts[0].pad.l_bumper);
    device.digital("Debugger", virtualPorts[0].pad.r_bumper);
    port.append(device); }

    ports.append(port);
  }
}

auto NeoGeoPocketColor::load() -> LoadResult {
  game = mia::Medium::create("Neo Geo Pocket Color");
  string location = Emulator::load(game, configuration.game);
  if(!location) return LoadResult(noFileSelected);
  LoadResult result = game->load(location);
  if(result != LoadResult(successful)) return result;

  system = mia::System::create("Neo Geo Pocket Color");
  result = system->load(firmware[0].location);
  if(result != LoadResult(successful)) {
    result.firmwareSystemName = "Neo Geo Pocket Color";
    result.firmwareType = firmware[0].type;
    result.firmwareRegion = firmware[0].region;
    result.result = noFirmware;
    return result;
  }

  if(!ares::NeoGeoPocket::load(root, "[SNK] Neo Geo Pocket Color")) return LoadResult(otherError);

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto fastBoot = root->find<ares::Node::Setting::Boolean>("Fast Boot")) {
    fastBoot->setValue(settings.boot.fast);
  }

  return LoadResult(successful);
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
