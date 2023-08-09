struct NeoGeoCD : Emulator {
  NeoGeoCD();
  auto load() -> LoadResult override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto arcade() -> bool override { return !name.find("Neo Geo CD"); }
};

NeoGeoCD::NeoGeoCD() {
  manufacturer = "SNK";
  name = "Neo Geo CD";
  medium = "Neo Geo CD";

  firmware.append({"BIOS", "World", "c11c33589b2057008a7e4c7c700fcd989a8c0f95f869e7e7539fdd00414d99a7"});

  for(auto id : range(2)) {
    InputPort port{string{"Controller Port ", 1 + id}};

  // Button layout mirrors Neo Geo CD pad
  { InputDevice device{"Arcade Stick"};
    device.digital("Up",     virtualPorts[id].pad.up);
    device.digital("Down",   virtualPorts[id].pad.down);
    device.digital("Left",   virtualPorts[id].pad.left);
    device.digital("Right",  virtualPorts[id].pad.right);
    device.digital("A",      virtualPorts[id].pad.south);
    device.digital("B",      virtualPorts[id].pad.east);
    device.digital("C",      virtualPorts[id].pad.west);
    device.digital("D",      virtualPorts[id].pad.north);
    device.digital("Select", virtualPorts[id].pad.select);
    device.digital("Start",  virtualPorts[id].pad.start);
    port.append(device); }

    ports.append(port);
  }
}

auto NeoGeoCD::load() -> LoadResult {
  game = mia::Medium::create("Neo Geo CD");
  auto result = game->load(Emulator::load(game, configuration.game));
  if(result != successful) return result;

  system = mia::System::create("Neo Geo CD");
  result = system->load(firmware[0].location);
  if(result != successful) {
    result.firmwareSystemName = "Neo Geo CD";
    result.firmwareType = firmware[0].type;
    result.firmwareRegion = firmware[0].region;
    result.result = noFirmware;
    return result;
  }

  if(!ares::NeoGeo::load(root, "[SNK] Neo Geo CD")) return otherError;

  if(auto port = root->find<ares::Node::Port>("Disc Tray")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    port->allocate("Arcade Stick");
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 2")) {
    port->allocate("Arcade Stick");
    port->connect();
  }

  return successful;
}

auto NeoGeoCD::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto NeoGeoCD::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Neo Geo CD") return system->pak;
  if(node->name() == "Neo Geo CD Disc") return game->pak;
  return {};
}
