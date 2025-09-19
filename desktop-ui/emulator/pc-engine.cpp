struct PCEngine : Emulator {
  PCEngine();
  auto load() -> LoadResult override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto allocatePorts() -> void;
  auto connectPorts() -> void;
};

PCEngine::PCEngine() {
  manufacturer = "NEC";
  name = "PC Engine";

  allocatePorts();
}

auto PCEngine::allocatePorts() -> void {
  ports.clear();

  InputPort port{string{"Controller Port"}};

  { InputDevice device{"Gamepad"};
    device.digital("Up",     virtualPorts[0].pad.up);
    device.digital("Down",   virtualPorts[0].pad.down);
    device.digital("Left",   virtualPorts[0].pad.left);
    device.digital("Right",  virtualPorts[0].pad.right);
    device.digital("II",     virtualPorts[0].pad.south);
    device.digital("I",      virtualPorts[0].pad.east);
    device.digital("Select", virtualPorts[0].pad.select);
    device.digital("Run",    virtualPorts[0].pad.start);
    port.append(device); }

  { InputDevice device{"Avenue Pad 6"};
    device.digital("Up",    virtualPorts[0].pad.up);
    device.digital("Down",  virtualPorts[0].pad.down);
    device.digital("Left",  virtualPorts[0].pad.left);
    device.digital("Right", virtualPorts[0].pad.right);
    device.digital("III",   virtualPorts[0].pad.west);
    device.digital("II",    virtualPorts[0].pad.south);
    device.digital("I",     virtualPorts[0].pad.east);
    device.digital("IV",    virtualPorts[0].pad.l_bumper);
    device.digital("V",     virtualPorts[0].pad.north);
    device.digital("VI",    virtualPorts[0].pad.r_bumper);
    device.digital("Select",virtualPorts[0].pad.select);
    device.digital("Run",   virtualPorts[0].pad.start);
    port.append(device); }

    ports.push_back(port);

  for(auto id : range(5)) {
    InputPort port{string{"Controller Port ", 1 + id}};

  { InputDevice device{"Gamepad"};
    device.digital("Up",     virtualPorts[id].pad.up);
    device.digital("Down",   virtualPorts[id].pad.down);
    device.digital("Left",   virtualPorts[id].pad.left);
    device.digital("Right",  virtualPorts[id].pad.right);
    device.digital("II",     virtualPorts[id].pad.south);
    device.digital("I",      virtualPorts[id].pad.east);
    device.digital("Select", virtualPorts[id].pad.select);
    device.digital("Run",    virtualPorts[id].pad.start);
    port.append(device); }

  { InputDevice device{"Avenue Pad 6"};
    device.digital("Up",    virtualPorts[id].pad.up);
    device.digital("Down",  virtualPorts[id].pad.down);
    device.digital("Left",  virtualPorts[id].pad.left);
    device.digital("Right", virtualPorts[id].pad.right);
    device.digital("III",   virtualPorts[id].pad.west);
    device.digital("II",    virtualPorts[id].pad.south);
    device.digital("I",     virtualPorts[id].pad.east);
    device.digital("IV",    virtualPorts[id].pad.l_bumper);
    device.digital("V",     virtualPorts[id].pad.north);
    device.digital("VI",    virtualPorts[id].pad.r_bumper);
    device.digital("Select",virtualPorts[id].pad.select);
    device.digital("Run",   virtualPorts[id].pad.start);
    port.append(device); }

    ports.push_back(port);
  }
}

auto PCEngine::connectPorts() -> void {
  if(auto port = root->find<ares::Node::Port>("Controller Port")) {
    port->allocate("Gamepad");
    port->connect();
  }
}

auto PCEngine::load() -> LoadResult {
  game = mia::Medium::create("PC Engine");
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;

  system = mia::System::create("PC Engine");
  result = system->load();
  if(result != successful) return result;

  ares::PCEngine::option("Pixel Accuracy", settings.video.pixelAccuracy);

  auto region = Emulator::region();
  string name = region == "NTSC-J" ? "PC Engine" : "TurboGrafx 16";
  if(!ares::PCEngine::load(root, {"[NEC] ", name, " (", region, ")"})) return otherError;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  connectPorts();

  return successful;
}

auto PCEngine::save() -> bool {
  root->save();
  system->save(game->location);
  game->save(game->location);
  return true;
}

auto PCEngine::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "PC Engine") return system->pak;
  if(node->name() == "PC Engine Card") return game->pak;
  return {};
}
