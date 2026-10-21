struct WonderSwan : Emulator {
  WonderSwan();
  auto load() -> LoadResult override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> std::shared_ptr<vfs::directory> override;
};

WonderSwan::WonderSwan() {
  manufacturer = "Bandai";
  name = "WonderSwan";

  { InputPort port{"WonderSwan"};

  { InputDevice device{"Controls"};
    device.digital("Y1",    virtualPorts[0].pad.l_bumper);
    device.digital("Y2",    virtualPorts[0].pad.l_trigger);
    device.digital("Y3",    virtualPorts[0].pad.r_bumper);
    device.digital("Y4",    virtualPorts[0].pad.r_trigger);
    device.digital("X1",    virtualPorts[0].pad.up);
    device.digital("X2",    virtualPorts[0].pad.right);
    device.digital("X3",    virtualPorts[0].pad.down);
    device.digital("X4",    virtualPorts[0].pad.left);
    device.digital("B",     virtualPorts[0].pad.south);
    device.digital("A",     virtualPorts[0].pad.east);
    device.digital("Start", virtualPorts[0].pad.start);
    port.append(device); }

    ports.push_back(port);
  }
}

auto WonderSwan::load() -> LoadResult {
  game = mia::Medium::create("WonderSwan");
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;

  system = mia::System::create("WonderSwan");
  result = system->load();
  if(result != successful) return result;

  ares::WonderSwan::option("Pixel Accuracy", settings.video.pixelAccuracy);

  if(!ares::WonderSwan::load(root, "[Bandai] WonderSwan")) return otherError;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  return successful;
}

auto WonderSwan::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto WonderSwan::pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> {
  if(node->name() == "WonderSwan") return system->pak;
  if(node->name() == "WonderSwan Cartridge") return game->pak;
  return {};
}
