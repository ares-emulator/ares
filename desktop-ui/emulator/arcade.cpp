struct Arcade : Emulator {
  Arcade();
  auto load() -> LoadResult override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> std::shared_ptr<vfs::directory> override;
  auto group() -> string override { return "Arcade"; }
  auto arcade() -> bool override { return true; }
  auto gameBrowserCategories() -> std::vector<GameBrowserCategory> override;
  auto input(ares::Node::Input::Input) -> void override;
  string systemPakName = "Arcade";
  string gamePakName = "Arcade";

  struct Mahjong {
    InputDigital a;
    InputDigital b;
    InputDigital c;
    InputDigital d;
    InputDigital e;
    InputDigital f;
    InputDigital g;
    InputDigital h;
    InputDigital i;
    InputDigital j;
    InputDigital k;
    InputDigital l;
    InputDigital m;
    InputDigital n;
    InputDigital kan;
    InputDigital pon;
    InputDigital chi;
    InputDigital reach;
    InputDigital ron;
  } mahjong;

  static auto available() -> bool;
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

    device.relative("X-Axis",  virtualPorts[0].mouse.x);
    device.relative("Y-Axis",  virtualPorts[0].mouse.y);
    device.digital ("Trigger", virtualPorts[0].mouse.left);

    port.append(device); }

  { InputDevice device{"Mahjong"};
    device.setMappingMode(MappingMode::Direct);
    device.digital("A",      mahjong.a);
    device.digital("B",      mahjong.b);
    device.digital("C",      mahjong.c);
    device.digital("D",      mahjong.d);
    device.digital("E",      mahjong.e);
    device.digital("F",      mahjong.f);
    device.digital("G",      mahjong.g);
    device.digital("H",      mahjong.h);
    device.digital("I",      mahjong.i);
    device.digital("J",      mahjong.j);
    device.digital("K",      mahjong.k);
    device.digital("L",      mahjong.l);
    device.digital("M",      mahjong.m);
    device.digital("N",      mahjong.n);
    device.digital("カン",    mahjong.kan);
    device.digital("ポン",    mahjong.pon);
    device.digital("チー",    mahjong.chi);
    device.digital("リーチ",  mahjong.reach);
    device.digital("ロン",    mahjong.ron);
    port.append(device); }

    ports.push_back(port);
  }
}

auto Arcade::available() -> bool {
#if defined(CORE_SG) || defined(CORE_N64) || defined(CORE_FC) || defined(CORE_NG)
  return true;
#endif
  return false;
}

auto Arcade::gameBrowserCategories() -> std::vector<GameBrowserCategory> {
  std::vector<GameBrowserCategory> categories;
#ifdef CORE_NG
  categories.push_back({"Neo Geo MVS", "snk/neogeo", "MVS"});
#endif
#ifdef CORE_FC
  categories.push_back({"Nintendo Vs. System", "nintendo/vs"});
#endif
#ifdef CORE_N64
  categories.push_back({"Nintendo Aleck 64", "nintendo/aleck64"});
#endif
#ifdef CORE_SG
  categories.push_back({"Sega SG-1000 Arcade", "sega/sg1000a"});
#endif
  return categories;
}

auto Arcade::load() -> LoadResult {
  systemPakName = "Arcade";
  gamePakName = "Arcade";
  game = mia::Medium::create("Arcade");
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;

#ifdef CORE_NG
  if(game->pak->attribute("arcade") == "neogeo") {
    system = mia::System::create("Neo Geo MVS");
    system->biosName = settings.neoGeo.mvsBios;
    auto bios = locateArcadeBios(location, "neogeo", "neogeo.zip");
    result = system->load(bios);
    if(result != successful) {
      return {romNotFound, {"Neo Geo MVS requires ", settings.neoGeo.mvsBios, " in neogeo.zip: ", bios}};
    }

    systemPakName = "Neo Geo MVS";
    gamePakName = "Neo Geo Cartridge";
    if(!ares::NeoGeo::load(root, "[SNK] Neo Geo MVS")) return otherError;
    if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
      port->allocate();
      port->connect();
    }
    for(auto id : range(2)) {
      if(auto port = root->find<ares::Node::Port>(string{"Controller Port ", id + 1})) {
        port->allocate("Arcade Stick");
        port->connect();
      }
    }
    if(auto port = root->find<ares::Node::Port>("Memory Card Slot")) {
      port->allocate("Memory Card");
      port->connect();
    }
    return successful;
  }
#endif

  system = mia::System::create("Arcade");
  result = system->load();
  if(result != successful) return result;

  //Determine from the game manifest which core to use for the given arcade rom
#ifdef CORE_FC
  if(game->pak->attribute("board") == "nintendo/vs") {
    if(game->pak->attribute("system") != "Vs. UniSystem") return otherError;
    if(!ares::Famicom::load(root, {"[Nintendo] Vs. UniSystem"})) return otherError;
    systemPakName = "Famicom";
    gamePakName = "Famicom Cartridge";

    if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
      port->allocate();
      port->connect();
    }
    return successful;
  }
#endif

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
      return otherError;
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

    ares::Nintendo64::option("Quality", settings.nintendo64.quality);
    ares::Nintendo64::option("Supersampling", settings.nintendo64.supersampling);
#if defined(VULKAN)
    ares::Nintendo64::option("Enable GPU acceleration", true);
#else
    ares::Nintendo64::option("Enable GPU acceleration", false);
#endif
    ares::Nintendo64::option("Disable Video Interface Processing", settings.nintendo64.disableVideoInterfaceProcessing);
    ares::Nintendo64::option("Weave Deinterlacing", settings.nintendo64.weaveDeinterlacing);
    ares::Nintendo64::option("Homebrew Mode", settings.developer.homebrewMode);
    ares::Nintendo64::option("Deterministic Entropy", settings.developer.deterministicEntropy);
    ares::Nintendo64::option("Recompiler", !settings.developer.forceInterpreter);

    return successful;
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

auto Arcade::pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> {
  if(node->name() == systemPakName) return system->pak;
  if(node->name() == gamePakName) return game->pak;
  return {};
}

auto Arcade::input(ares::Node::Input::Input input) -> void {
  auto device = ares::Node::parent(input);
  if(!device) return;

  auto port = ares::Node::parent(device);
  if(!port) return;
  if(device->name() == "Arcade Stick" && (port->name() == "Controller Port 1" || port->name() == "Controller Port 2")) {
    auto player = port->name() == "Controller Port 1" ? 1 : 2;
    auto name = input->name();
    if(name == "A") name = "Button 1";
    if(name == "B") name = "Button 2";
    if(name == "C") name = "Button 3";
    if(name == "D") name = "Button 4";
    if(name == "Select") name = "Coin";
    return Emulator::input(input, "Arcade", "Controls", {"Player ", player, " ", name});
  }
  return Emulator::input(input);
}
