struct Arcade : Emulator {
  Arcade();
  auto load() -> LoadResult override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> std::shared_ptr<vfs::directory> override;
  auto group() -> string override { return "Arcade"; }
  auto arcade() -> bool override { return true; }
  auto gameBrowserCategories() -> std::vector<GameBrowserCategory> override;
  auto input(ares::Node::Input::Input) -> void override;
  auto buildInputs(string manifest) -> void;
  auto buildDefaultInputs() -> void;
  auto labelInputs(InputPort& port) -> void;
  auto reloadInputBindings() -> void;
  auto buildInputScopes(string game, string title) -> void;
  InputDigital unmapped;
  string systemPakName = "Arcade";
  string gamePakName = "Arcade";
  string currentPortName = "Arcade";

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
  buildDefaultInputs();
}

auto Arcade::labelInputs(InputPort& port) -> void {
  for(auto& device : port.devices) {
    for(auto& input : device.inputs) {
      if(input.label) continue;  //Preserve labels supplied by the MAME driver.
      input.label = input.name;
      if(input.name == "Test") input.label = "Service Mode";
      if(input.name == "Service") input.label = "Service 1";
      if(!input.name.beginsWith("Player ")) continue;
      auto parts = nall::split(input.name, " ", 2);
      if(parts.size() != 3) continue;
      auto& player = parts[1];
      auto& action = parts[2];
      if(action == "Coin") input.label = {"Coin ", player};
      else if(action == "Start") input.label = {player, player == "1" ? " Player Start" : " Players Start"};
      else input.label = {"P", player, " ", action};
    }
  }
}

auto Arcade::buildDefaultInputs() -> void {
  ports.clear();
  currentPortName = "General";

  InputPort port{currentPortName};
  port.settingsKey = "Arcade";

  { InputDevice device{"Controls"};
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
      device.analog ({"Player ", n + 1, " Accelerator"}, virtualPorts[n].pad.r_trigger);
    }

    device.relative("X-Axis",  virtualPorts[0].mouse.x);
    device.relative("Y-Axis",  virtualPorts[0].mouse.y);
    device.digital ("Trigger", virtualPorts[0].mouse.left);

    port.append(device);
  }

  { InputDevice device{"Mahjong"};
    device.setMappingMode(MappingMode::VirtualPad);
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
    port.append(device);
  }

  labelInputs(port);
  ports.push_back(port);
}

auto Arcade::buildInputs(string manifest) -> void {
  if(!manifest) return buildDefaultInputs();

  auto document = BML::unserialize(manifest);
  auto games = document.find("game");
  if(games.size() != 1) return buildDefaultInputs();
  auto inputNode = games.front()["input"];
  string gameTitle = games.front()["title"].string();
  if(!gameTitle) gameTitle = "Arcade";
  if(!inputNode) {
    settings.process(false);
    buildDefaultInputs();
    buildInputScopes(games.front()["name"].string(), gameTitle);
    reloadInputBindings();
    return;
  }

  u32 players = inputNode["players"].natural();
  if(!players) players = 2;
  if(players > 5) players = 5;

  struct ControlDef {
    string type;
    u32 player = 0;
    u32 buttons = 0;
    string ways;
  };
  std::vector<ControlDef> controls;
  for(auto control : inputNode) {
    if(control.name() != "control") continue;
    ControlDef def;
    def.type = control["type"].string();
    def.player = control["player"].natural();
    def.buttons = control["buttons"].natural();
    def.ways = control["ways"].string();
    controls.push_back(def);
  }

  settings.process(false);  // Keep outgoing overrides before rebuilding active scopes.
  ports.clear();
  currentPortName = gameTitle;
  InputPort port{currentPortName};

  auto actions = inputNode.find("action");
  bool hasActions = actions.size() > 0;

  auto resolveMapping = [&](string binding, u32 player) -> InputMapping* {
    if(player < 1 || player > 5) player = 1;
    auto& pad = virtualPorts[player - 1].pad;
    if(binding == "up") return &pad.up;
    if(binding == "down") return &pad.down;
    if(binding == "left") return &pad.left;
    if(binding == "right") return &pad.right;
    if(binding == "south") return &pad.south;
    if(binding == "east") return &pad.east;
    if(binding == "west") return &pad.west;
    if(binding == "north") return &pad.north;
    if(binding == "l_bumper") return &pad.l_bumper;
    if(binding == "r_bumper") return &pad.r_bumper;
    if(binding == "l_trigger") return &pad.l_trigger;
    if(binding == "r_trigger") return &pad.r_trigger;
    if(binding == "start") return &pad.start;
    if(binding == "select") return &pad.select;
    if(binding == "lstick_click") return &pad.lstick_click;
    if(binding == "rstick_click") return &pad.rstick_click;
    return nullptr;
  };

  { InputDevice device{"Controls"};
    if(hasActions) {
      for(auto action : actions) {
        auto token = action["token"].string();
        auto name = action["name"].string();
        auto label = action["label"].string();
        auto player = action["player"].natural();
        auto binding = action["default"].string();
        if(!token || !name) continue;

        u32 p = player >= 1 && player <= 5 ? player - 1 : 0;
        auto& pad = virtualPorts[p].pad;
        bool pedal = token.endsWith("_PEDAL");
        bool axisY = token.endsWith("_TRACKBALL_Y") || token.endsWith("_LIGHTGUN_Y");
        bool axisX = token.endsWith("_PADDLE") || token.endsWith("_DIAL")
          || token.endsWith("_TRACKBALL_X") || token.endsWith("_LIGHTGUN_X") || token.endsWith("_POSITIONAL");
        if(pedal) {
          device.analog(name, pad.r_trigger);
          device.inputs.back().label = {"P", p + 1, " Accelerator"};
          continue;
        }
        if(axisX || axisY) {
          auto& lo = axisY ? pad.lstick_up : pad.lstick_left;
          auto& hi = axisY ? pad.lstick_down : pad.lstick_right;
          bool steering = games.front()["name"].string() == "hangonjr" && axisX;
          string axisLabel = steering ? string{"P", p + 1, " Steering"} : label ? label : name;
          device.analog({name, " -"}, lo);
          device.inputs.back().label = {axisLabel, axisY ? " Up" : " Left"};
          device.analog({name, " +"}, hi);
          device.inputs.back().label = {axisLabel, axisY ? " Down" : " Right"};
          device.analog(name, lo, hi);
          continue;
        }

        auto* mapping = resolveMapping(binding, player);
        if(!mapping) mapping = &unmapped;
        device.digital(name, *mapping);
        auto& input = device.inputs.back();
        input.label = label;
      }
    } else {
      device.digital("Service", virtualPorts[0].pad.lstick_click);
      device.digital("Test",    virtualPorts[0].pad.rstick_click);

      for(auto n : range(players)) {
        u32 buttonCount = 0;
        string controlType = "joy";
        for(auto& def : controls) {
          if(def.player == n + 1 || (def.player == 0 && n == 0)) {
            if(def.buttons > buttonCount) buttonCount = def.buttons;
            controlType = def.type;
          }
        }
        if(!buttonCount) {
          for(auto& def : controls) {
            if(def.buttons > buttonCount) buttonCount = def.buttons;
            controlType = def.type;
          }
        }
        if(buttonCount > 8) buttonCount = 8;

        if(controlType == "lightgun" || controlType == "positional") {
          device.relative({"Player ", n + 1, " X-Axis"}, virtualPorts[n].mouse.x);
          device.relative({"Player ", n + 1, " Y-Axis"}, virtualPorts[n].mouse.y);
          device.digital ({"Player ", n + 1, " Trigger"}, virtualPorts[n].mouse.left);
        } else if(controlType == "trackball" || controlType == "dial" || controlType == "paddle") {
          device.digital({"Player ", n + 1, " Up"   }, virtualPorts[n].pad.up);
          device.digital({"Player ", n + 1, " Down" }, virtualPorts[n].pad.down);
          device.digital({"Player ", n + 1, " Left" }, virtualPorts[n].pad.left);
          device.digital({"Player ", n + 1, " Right"}, virtualPorts[n].pad.right);
          device.analog ({"Player ", n + 1, " X-Axis -"}, virtualPorts[n].pad.lstick_left);
          device.analog ({"Player ", n + 1, " X-Axis +"}, virtualPorts[n].pad.lstick_right);
          device.analog ({"Player ", n + 1, " Y-Axis -"}, virtualPorts[n].pad.lstick_up);
          device.analog ({"Player ", n + 1, " Y-Axis +"}, virtualPorts[n].pad.lstick_down);
          device.analog ({"Player ", n + 1, " X-Axis"  }, virtualPorts[n].pad.lstick_left, virtualPorts[n].pad.lstick_right);
          device.analog ({"Player ", n + 1, " Y-Axis"  }, virtualPorts[n].pad.lstick_up,   virtualPorts[n].pad.lstick_down);
        } else {
          device.digital({"Player ", n + 1, " Up"   }, virtualPorts[n].pad.up);
          device.digital({"Player ", n + 1, " Down" }, virtualPorts[n].pad.down);
          device.digital({"Player ", n + 1, " Left" }, virtualPorts[n].pad.left);
          device.digital({"Player ", n + 1, " Right"}, virtualPorts[n].pad.right);
        }

        InputMapping* buttonMappings[] = {
          &virtualPorts[n].pad.west,
          &virtualPorts[n].pad.south,
          &virtualPorts[n].pad.east,
          &virtualPorts[n].pad.north,
          &virtualPorts[n].pad.l_bumper,
          &virtualPorts[n].pad.r_bumper,
          &virtualPorts[n].pad.l_trigger,
          &virtualPorts[n].pad.r_trigger,
        };
        for(u32 b : range(buttonCount)) {
          device.digital({"Player ", n + 1, " Button ", b + 1}, *buttonMappings[b]);
        }

        device.digital({"Player ", n + 1, " Start"}, virtualPorts[n].pad.start);
        device.digital({"Player ", n + 1, " Coin" }, virtualPorts[n].pad.select);
      }
    }
    port.append(device);
  }

  { InputDevice device{"Mahjong"};
    device.setMappingMode(MappingMode::VirtualPad);
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
    port.append(device);
  }

  labelInputs(port);
  ports.push_back(port);
  buildInputScopes(games.front()["name"].string(), gameTitle);
  reloadInputBindings();
}

auto Arcade::buildInputScopes(string gameName, string title) -> void {
  auto gamePort = std::move(ports.back());
  buildDefaultInputs();
  currentPortName = "Arcade";
  gamePort.name = currentPortName;
  gamePort.settingsKey = {"Game.", Settings::inputKey(gameName)};
  gameInputPorts = {std::move(gamePort)};
  gameInputTitle = title;
  gameInputKey = Settings::inputKey(string{gameName}.downcase());

  for(auto& device : gameInputPorts.front().devices) {
    for(auto& source : ports.front().devices) {
      if(source.name != device.name) continue;
      for(auto& input : device.inputs) {
        for(auto& inherited : source.inputs) {
          if(input.name == inherited.name && input.type == inherited.type && inherited.directMapping)
            input.inheritedMappings.push_back(inherited.directMapping);
        }
      }
      for(auto& pair : device.pairs) {
        for(auto& inherited : source.pairs) {
          if(pair.name != inherited.name) continue;
          pair.inheritedMappingsLo.push_back(inherited.directMappingLo);
          pair.inheritedMappingsHi.push_back(inherited.directMappingHi);
        }
      }
    }
  }
}

auto Arcade::reloadInputBindings() -> void {
  string base = "Arcade";
  for(auto* scopes : {&ports, &gameInputPorts}) {
    for(auto& port : *scopes) {
      for(auto& device : port.devices) {
        if(!device.hasDirectMappings()) continue;
        string portName = port.settingsKey ? port.settingsKey : Settings::inputKey(port.name);
        string deviceName = Settings::inputKey(device.name);
        for(auto& input : device.inputs) {
          auto& mapping = input.configuredMapping();
          string inputName = Settings::inputKey(input.name, true);
          string key = {base, "/Input/", portName, "/", deviceName, "/", inputName};
          if(auto node = settings[key]) {
            string value = node.string();
            auto parts = nall::split(value, ";");
            parts.resize(BindingLimit);
            for(u32 binding : range(BindingLimit)) mapping.assignments[binding] = parts[binding];
          }
        }
        for(auto& pair : device.pairs) {
          string pairName = Settings::inputKey(pair.name, true);
          for(auto index : range(2)) {
            string suffix = index == 0 ? "Lo" : "Hi";
            auto& mapping = index == 0 ? pair.configuredMappingLo() : pair.configuredMappingHi();
            string key = {base, "/Input/", portName, "/", deviceName, "/", pairName, "/", suffix};
            if(auto node = settings[key]) {
              string value = node.string();
              auto parts = nall::split(value, ";");
              parts.resize(BindingLimit);
              for(u32 binding : range(BindingLimit)) mapping.assignments[binding] = parts[binding];
            }
          }
        }
      }
    }
  }
  //Migrate the previous arcade game scope once, then use the common game settings.
  if(auto parent = settings["Arcade/Input"]) {
    if(auto previous = parent[gameInputPorts.front().settingsKey]) parent.remove(previous);
  }
  processGameInputMappings(true);
  processGameInputMappings(false);
  inputManager.bind();
}

auto Arcade::available() -> bool {
#if defined(CORE_SG) || defined(CORE_N64) || defined(CORE_FC) || defined(CORE_MS) || defined(CORE_NG)
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
#ifdef CORE_MS
  categories.push_back({"Sega System E", "sega/segae"});
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

  buildInputs(game->manifest);

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
#ifdef CORE_MS
  if(game->pak->attribute("board") == "sega/segae") {
    systemPakName = "System E";
    gamePakName = "System E Cartridge";
    if(!ares::MasterSystem::load(root, "[Sega] System E")) return otherError;
    if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
      port->allocate();
      port->connect();
    }
    return successful;
  }
#endif
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
    return Emulator::input(input, currentPortName, "Controls", {"Player ", player, " ", name});
  }
  if(device->name() == "Controls" || device->name() == "Mahjong") {
    return Emulator::input(input, currentPortName, device->name(), input->name());
  }
  return Emulator::input(input);
}
