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

  { InputDevice device{"Controls"} ;
    device.digital("Service", virtualPorts[0].pad.lstick_click);
    device.digital("Test",    virtualPorts[0].pad.rstick_click);
    for(auto n : range(2)) {
      device.digital({"Player ", n + 1, " Up"      }, virtualPorts[n].pad.up);
      device.digital({"Player ", n + 1, " Down"    }, virtualPorts[n].pad.down);
      device.digital({"Player ", n + 1, " Left"    }, virtualPorts[n].pad.left);
      device.digital({"Player ", n + 1, " Right"   }, virtualPorts[n].pad.right);
      device.digital({"Player ", n + 1, " Button 1"}, virtualPorts[n].pad.west);
      device.digital({"Player ", n + 1, " Button 2"}, virtualPorts[n].pad.south);
      device.digital({"Player ", n + 1, " Button 3"}, virtualPorts[n].pad.east);
      device.digital({"Player ", n + 1, " Button 4"}, virtualPorts[n].pad.north);
      device.digital({"Player ", n + 1, " Button 5"}, virtualPorts[n].pad.l_bumper);
      device.digital({"Player ", n + 1, " Button 6"}, virtualPorts[n].pad.r_bumper);
      device.digital({"Player ", n + 1, " Button 7"}, virtualPorts[n].pad.l_trigger);
      device.digital({"Player ", n + 1, " Button 8"}, virtualPorts[n].pad.r_trigger);
      device.digital({"Player ", n + 1, " Start"   }, virtualPorts[n].pad.start);
      device.digital({"Player ", n + 1, " Coin"    }, virtualPorts[n].pad.select);
      device.analog ({"Player ", n + 1, " Y-Axis -"}, virtualPorts[n].pad.lstick_up);
      device.analog ({"Player ", n + 1, " Y-Axis +"}, virtualPorts[n].pad.lstick_down);
      device.analog ({"Player ", n + 1, " X-Axis -"}, virtualPorts[n].pad.lstick_left);
      device.analog ({"Player ", n + 1, " X-Axis +"}, virtualPorts[n].pad.lstick_right);
      device.analog ({"Player ", n + 1, " X-Axis"  }, virtualPorts[n].pad.lstick_left, virtualPorts[n].pad.lstick_right);
      device.analog ({"Player ", n + 1, " Y-Axis"  }, virtualPorts[n].pad.lstick_up,   virtualPorts[n].pad.lstick_down);
    }
    port.append(device); }

    ports.append(port);
  }
}

auto Arcade::load() -> LoadResult {
  game = mia::Medium::create("Arcade");
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;

  system = mia::System::create("Arcade");
  result = system->load();
  if(result != successful) return result;

  //Determine from the game manifest which core to use for the given arcade rom
#ifdef CORE_SG
  if(game->pak->attribute("board") == "sega/sg1000a") {
    if(!ares::SG1000::load(root, {"[Sega] SG-1000A"})) return otherError;
    systemPakName = "SG-1000A";
    gamePakName = "Arcade Cartridge";

    if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
      port->allocate();
      port->connect();
    }
    return successful;
  }
#endif

#ifdef CORE_N64
  if(game->pak->attribute("board") == "nintendo/aleck64") {
    if(!ares::Nintendo64::load(root, {"[SETA] Aleck 64"})) {
      return false;
    }
    systemPakName = "Aleck 64";
    gamePakName = "Arcade Cartridge";

    if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
      port->allocate();
      port->connect();
    }

    if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
      port->allocate("Aleck64");
      port->connect();
    }

    if(auto port = root->find<ares::Node::Port>("Controller Port 2")) {
      port->allocate("Aleck64");
      port->connect();
    }

    ares::Nintendo64::option("Quality", settings.video.quality);
    ares::Nintendo64::option("Supersampling", settings.video.supersampling);
#if defined(VULKAN)
    ares::Nintendo64::option("Enable GPU acceleration", true);
#else
    ares::Nintendo64::option("Enable GPU acceleration", false);
#endif
    ares::Nintendo64::option("Disable Video Interface Processing", settings.video.disableVideoInterfaceProcessing);
    ares::Nintendo64::option("Weave Deinterlacing", settings.video.weaveDeinterlacing);
    ares::Nintendo64::option("Homebrew Mode", settings.general.homebrewMode);
    ares::Nintendo64::option("Recompiler", !settings.general.forceInterpreter);

    return true;
  }
#endif

  return otherError;
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
