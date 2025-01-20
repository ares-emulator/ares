struct Arcade : Emulator {
  Arcade();
  auto load() -> LoadResult override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto group() -> string override { return "Arcade"; }
  auto arcade() -> bool override { return true; }
  string systemPakName = "Arcade";
  string gamePakName = "Arcade";
};

Arcade::Arcade() {
  manufacturer = "Arcade";
  name = "Arcade";
  medium = "Arcade";

  { InputPort port{string{"Arcade"}};

  { InputDevice device{"Controls"};
    for(auto n : range(2)) {
      device.digital({"Player ", n + 1, " Up"      }, virtualPorts[n].pad.up);
      device.digital({"Player ", n + 1, " Down"    }, virtualPorts[n].pad.down);
      device.digital({"Player ", n + 1, " Left"    }, virtualPorts[n].pad.left);
      device.digital({"Player ", n + 1, " Right"   }, virtualPorts[n].pad.right);
      device.digital({"Player ", n + 1, " Button 1"}, virtualPorts[n].pad.south);
      device.digital({"Player ", n + 1, " Button 2"}, virtualPorts[n].pad.east);
      device.digital({"Player ", n + 1, " Start"   }, virtualPorts[n].pad.start);
      device.digital({"Player ", n + 1, " Coin"    }, virtualPorts[n].pad.select);
    }
    port.append(device); }

    ports.append(port);
  }
}

auto Arcade::load() -> LoadResult {
  game = mia::Medium::create("Arcade");
  string location = Emulator::load(game, configuration.game);
  if(!location) return LoadResult(noFileSelected);
  LoadResult result = game->load(location);
  if(result != LoadResult(successful)) return result;

  system = mia::System::create("Arcade");
  result = system->load();
  if(result != LoadResult(successful)) return result;

  //Determine from the game manifest which core to use for the given arcade rom
#ifdef CORE_SG
  if(game->pak->attribute("board") == "sega/sg1000a") {
    if(!ares::SG1000::load(root, {"[Sega] SG-1000A"})) return LoadResult(otherError);
    systemPakName = "SG-1000A";
    gamePakName = "Arcade Cartridge";

    if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
      port->allocate();
      port->connect();
    }
    return LoadResult(successful);
  }
#endif

  return LoadResult(otherError);
}

auto Arcade::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto Arcade::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == systemPakName) return system->pak;
  if(node->name() == gamePakName) return game->pak;
  return {};
}
