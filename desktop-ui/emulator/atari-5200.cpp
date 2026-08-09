struct Atari5200 : Emulator {
  Atari5200();
  auto load() -> LoadResult override;
  auto save() -> bool override;
  auto pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> override;
};

Atari5200::Atari5200() {
  manufacturer = "Atari";
  name = "Atari 5200";

  firmware.push_back({
    "BIOS",
    "NTSC-U Four-port",
    "06b250f18983d058c0f156ce7ee88ae48b6eaf11e6f10f21dccf6ac7ffb6a6af"
  });

  for(auto id : range(4)) {
    InputPort port{string{"Controller Port ", 1 + id}};

  { InputDevice device{"Controller"};
    device.analog ("L-Up",       virtualPorts[id].pad.lstick_up);
    device.analog ("L-Down",     virtualPorts[id].pad.lstick_down);
    device.analog ("L-Left",     virtualPorts[id].pad.lstick_left);
    device.analog ("L-Right",    virtualPorts[id].pad.lstick_right);
    device.analog ("X-Axis",     virtualPorts[id].pad.lstick_left, virtualPorts[id].pad.lstick_right);
    device.analog ("Y-Axis",     virtualPorts[id].pad.lstick_up,   virtualPorts[id].pad.lstick_down);
    device.digital("Top Fire",   virtualPorts[id].pad.east);
    device.digital("Bottom Fire", virtualPorts[id].pad.south);
    device.digital("1",          virtualPorts[id].pad.one);
    device.digital("2",          virtualPorts[id].pad.two);
    device.digital("3",          virtualPorts[id].pad.three);
    device.digital("4",          virtualPorts[id].pad.four);
    device.digital("5",          virtualPorts[id].pad.five);
    device.digital("6",          virtualPorts[id].pad.six);
    device.digital("7",          virtualPorts[id].pad.seven);
    device.digital("8",          virtualPorts[id].pad.eight);
    device.digital("9",          virtualPorts[id].pad.nine);
    device.digital("*",          virtualPorts[id].pad.star);
    device.digital("0",          virtualPorts[id].pad.zero);
    device.digital("#",          virtualPorts[id].pad.pound);
    device.digital("Start",      virtualPorts[id].pad.start);
    device.digital("Pause",      virtualPorts[id].pad.select);
    device.digital("Reset",      virtualPorts[id].pad.north);
    port.append(device); }

    ports.push_back(port);
  }
}

auto Atari5200::load() -> LoadResult {
  game = mia::Medium::create("Atari 5200");
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;

  LoadResult result = game->load(location);
  if(result != successful) return result;

  system = mia::System::create("Atari 5200");
  if(system->load(firmware[0].location) != successful) {
    result.firmwareSystemName = "Atari 5200";
    result.firmwareType = firmware[0].type;
    result.firmwareRegion = firmware[0].region;
    result.result = noFirmware;
    return result;
  }

  if(!ares::Atari5200::load(root, "[Atari] Atari 5200 (NTSC)")) return otherError;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  for(auto id : range(4)) {
    if(auto port = root->find<ares::Node::Port>(string{"Controller Port ", 1 + id})) {
      port->allocate("Controller");
      port->connect();
    }
  }

  return successful;
}

auto Atari5200::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto Atari5200::pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> {
  if(node->name() == "Atari 5200") return system->pak;
  if(node->name() == "Atari 5200 Cartridge") return game->pak;
  return {};
}
