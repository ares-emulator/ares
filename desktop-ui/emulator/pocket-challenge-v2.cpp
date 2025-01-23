struct PocketChallengeV2 : Emulator {
  PocketChallengeV2();
  auto load(Menu) -> void override;
  auto load() -> LoadResult override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
};

PocketChallengeV2::PocketChallengeV2() {
  manufacturer = "Benesse";
  name = "Pocket Challenge V2";

  { InputPort port{"Pocket Challenge V2"};

  { InputDevice device{"Controls"};
    device.digital("Up",     virtualPorts[0].pad.up);
    device.digital("Down",   virtualPorts[0].pad.down);
    device.digital("Left",   virtualPorts[0].pad.left);
    device.digital("Right",  virtualPorts[0].pad.right);
    device.digital("Pass",   virtualPorts[0].pad.west);
    device.digital("Circle", virtualPorts[0].pad.south);
    device.digital("Clear",  virtualPorts[0].pad.east);
    device.digital("View",   virtualPorts[0].pad.start);
    device.digital("Escape", virtualPorts[0].pad.select);
    port.append(device); }

    ports.append(port);
  }
}

auto PocketChallengeV2::load(Menu menu) -> void {
  //the Pocket Challenge V2 game library is very small.
  //no titles for the system use portrait (vertical) orientation.
  //as such, neither the ares::WonderSwan core nor desktop-ui provide an orientation setting.
}

auto PocketChallengeV2::load() -> LoadResult {
  game = mia::Medium::create("Pocket Challenge V2");
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;

  system = mia::System::create("Pocket Challenge V2");
  result = system->load();
  if(result != successful) return result;

  ares::WonderSwan::option("Pixel Accuracy", settings.video.pixelAccuracy);

  if(!ares::WonderSwan::load(root, "[Benesse] Pocket Challenge V2")) return otherError;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  return successful;
}

auto PocketChallengeV2::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto PocketChallengeV2::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Pocket Challenge V2") return system->pak;
  if(node->name() == "Pocket Challenge V2 Cartridge") return game->pak;
  return {};
}
