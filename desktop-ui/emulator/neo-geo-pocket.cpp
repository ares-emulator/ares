struct NeoGeoPocket : Emulator {
  NeoGeoPocket();
  auto load() -> LoadResult override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> std::shared_ptr<vfs::directory> override;
};

NeoGeoPocket::NeoGeoPocket() {
  manufacturer = "SNK";
  name = "Neo Geo Pocket";

  firmware.push_back({"BIOS", "World", "0293555b21c4fac516d25199df7809b26beeae150e1d4504a050db32264a6ad7"});

  { InputPort port{"Neo Geo Pocket"};

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

    ports.push_back(port);
  }
}

auto NeoGeoPocket::load() -> LoadResult {
  game = mia::Medium::create("Neo Geo Pocket");
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;

  system = mia::System::create("Neo Geo Pocket");
  result = system->load(firmware[0].location);
  if(result != successful) {
    result.firmwareSystemName = "Neo Geo Pocket";
    result.firmwareType = firmware[0].type;
    result.firmwareRegion = firmware[0].region;
    result.result = noFirmware;
    return result;
  }

  if(!ares::NeoGeoPocket::load(root, "[SNK] Neo Geo Pocket")) return otherError;;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto fastBoot = root->find<ares::Node::Setting::Boolean>("Fast Boot")) {
    fastBoot->setValue(settings.boot.fast);
  }

  return successful;
}

auto NeoGeoPocket::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto NeoGeoPocket::pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> {
  if(node->name() == "Neo Geo Pocket") return system->pak;
  if(node->name() == "Neo Geo Pocket Cartridge") return game->pak;
  return {};
}
