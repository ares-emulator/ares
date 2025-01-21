struct NeoGeoMVS : Emulator {
  NeoGeoMVS();
  auto load() -> LoadResult override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto group() -> string override { return "Arcade"; }
  auto arcade() -> bool override { return true; }
};

NeoGeoMVS::NeoGeoMVS() {
  manufacturer = "SNK";
  name = "Neo Geo MVS";
  medium = "Neo Geo";

  firmware.append({"BIOS", "World"});

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

auto NeoGeoMVS::load() -> LoadResult {
  game = mia::Medium::create("Neo Geo");
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;

  system = mia::System::create("Neo Geo MVS");
  result = system->load(firmware[0].location);
  if(result != successful) {
    result.firmwareSystemName = "Neo Geo MVS";
    result.firmwareType = firmware[0].type;
    result.firmwareRegion = firmware[0].region;
    result.result = noFirmware;
    return result;
  }

  if(!ares::NeoGeo::load(root, "[SNK] Neo Geo MVS")) return otherError;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
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

  if(auto port = root->find<ares::Node::Port>("Memory Card Slot")) {
    port->allocate("Memory Card");
    port->connect();
  }

  return successful;
}

auto NeoGeoMVS::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto NeoGeoMVS::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Neo Geo MVS") return system->pak;
  if(node->name() == "Neo Geo Cartridge") return game->pak;
  return {};
}
