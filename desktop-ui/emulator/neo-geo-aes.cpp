struct NeoGeoAES : Emulator {
  NeoGeoAES();
  auto load() -> LoadResult override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> std::shared_ptr<vfs::directory> override;
  auto arcade() -> bool override { return true; }
  auto gameBrowserCategories() -> std::vector<GameBrowserCategory> override { return {{"Neo Geo", "snk/neogeo", "AES"}}; }
};

NeoGeoAES::NeoGeoAES() {
  manufacturer = "SNK";
  name = "Neo Geo AES";
  medium = "Neo Geo";

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

    ports.push_back(port);
  }
}

auto NeoGeoAES::load() -> LoadResult {
  game = mia::Medium::create("Neo Geo");
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;

  system = mia::System::create("Neo Geo AES");
  system->biosName = settings.neoGeo.aesBios;
  auto bios = locateArcadeBios(location, "neogeo", "aes.zip");
  result = system->load(bios);
  if(result != successful) {
    return {romNotFound, {"Neo Geo AES requires ", settings.neoGeo.aesBios, " in aes.zip: ", bios}};
  }

  if(!ares::NeoGeo::load(root, "[SNK] Neo Geo AES")) return otherError;

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

auto NeoGeoAES::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto NeoGeoAES::pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> {
  if(node->name() == "Neo Geo AES") return system->pak;
  if(node->name() == "Neo Geo Cartridge") return game->pak;
  return {};
}
