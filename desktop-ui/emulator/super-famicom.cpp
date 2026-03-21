struct SuperFamicom : Emulator {
  SuperFamicom();
  auto load() -> LoadResult override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> std::shared_ptr<vfs::directory> override;

  struct GamepadMappings {
    InputDigital up;
    InputDigital down;
    InputDigital left;
    InputDigital right;
    InputDigital b;
    InputDigital a;
    InputDigital y;
    InputDigital x;
    InputDigital l;
    InputDigital r;
    InputDigital select;
    InputDigital start;
  };

  struct RumbleGamepadMappings {
    InputDigital up;
    InputDigital down;
    InputDigital left;
    InputDigital right;
    InputDigital b;
    InputDigital a;
    InputDigital y;
    InputDigital x;
    InputDigital l;
    InputDigital r;
    InputDigital select;
    InputDigital start;
    InputRumble rumble;
  };

  struct PointerMappings {
    InputRelative x;
    InputRelative y;
    InputDigital left;
    InputDigital middle;
    InputDigital right;
  };

  struct JustifierMappings {
    InputRelative x;
    InputRelative y;
    InputDigital trigger;
    InputDigital start;
  };

  struct SuperScopeMappings {
    InputRelative x;
    InputRelative y;
    InputDigital trigger;
    InputDigital cursor;
    InputDigital turbo;
    InputDigital pause;
  };

  struct NttDataKeypadMappings {
    InputDigital up;
    InputDigital down;
    InputDigital left;
    InputDigital right;
    InputDigital b;
    InputDigital a;
    InputDigital y;
    InputDigital x;
    InputDigital l;
    InputDigital r;
    InputDigital back;
    InputDigital next;
    InputDigital one;
    InputDigital two;
    InputDigital three;
    InputDigital four;
    InputDigital five;
    InputDigital six;
    InputDigital seven;
    InputDigital eight;
    InputDigital nine;
    InputDigital zero;
    InputDigital star;
    InputDigital clear;
    InputDigital pound;
    InputDigital point;
    InputDigital end;
  };

  struct TwinTapMappings {
    InputDigital one;
    InputDigital two;
  };

  GamepadMappings gamepadMappings[2];
  RumbleGamepadMappings rumbleGamepadMappings[2];
  PointerMappings mouseMappings[2];
  JustifierMappings justifierMappings[2];
  SuperScopeMappings superScopeMappings[2];
  NttDataKeypadMappings keypadMappings[2];
  TwinTapMappings twinTapMappings[2];
  std::shared_ptr<mia::Pak> gb, bs, stA, stB;
};

SuperFamicom::SuperFamicom() {
  manufacturer = "Nintendo";
  name = "Super Famicom";

  for(auto id : range(2)) {
    auto& gamepad = gamepadMappings[id];
    gamepad.up.fallback = &virtualPorts[id].pad.up;
    gamepad.down.fallback = &virtualPorts[id].pad.down;
    gamepad.left.fallback = &virtualPorts[id].pad.left;
    gamepad.right.fallback = &virtualPorts[id].pad.right;
    gamepad.b.fallback = &virtualPorts[id].pad.south;
    gamepad.a.fallback = &virtualPorts[id].pad.east;
    gamepad.y.fallback = &virtualPorts[id].pad.west;
    gamepad.x.fallback = &virtualPorts[id].pad.north;
    gamepad.l.fallback = &virtualPorts[id].pad.l_bumper;
    gamepad.r.fallback = &virtualPorts[id].pad.r_bumper;
    gamepad.select.fallback = &virtualPorts[id].pad.select;
    gamepad.start.fallback = &virtualPorts[id].pad.start;

    auto& rumbleGamepad = rumbleGamepadMappings[id];
    rumbleGamepad.up.fallback = &virtualPorts[id].pad.up;
    rumbleGamepad.down.fallback = &virtualPorts[id].pad.down;
    rumbleGamepad.left.fallback = &virtualPorts[id].pad.left;
    rumbleGamepad.right.fallback = &virtualPorts[id].pad.right;
    rumbleGamepad.b.fallback = &virtualPorts[id].pad.south;
    rumbleGamepad.a.fallback = &virtualPorts[id].pad.east;
    rumbleGamepad.y.fallback = &virtualPorts[id].pad.west;
    rumbleGamepad.x.fallback = &virtualPorts[id].pad.north;
    rumbleGamepad.l.fallback = &virtualPorts[id].pad.l_bumper;
    rumbleGamepad.r.fallback = &virtualPorts[id].pad.r_bumper;
    rumbleGamepad.select.fallback = &virtualPorts[id].pad.select;
    rumbleGamepad.start.fallback = &virtualPorts[id].pad.start;
    rumbleGamepad.rumble.fallback = &virtualPorts[id].pad.rumble;

    auto& mouse = mouseMappings[id];
    mouse.x.fallback = &virtualPorts[id].mouse.x;
    mouse.y.fallback = &virtualPorts[id].mouse.y;
    mouse.left.fallback = &virtualPorts[id].mouse.left;
    mouse.right.fallback = &virtualPorts[id].mouse.right;

    auto& justifier = justifierMappings[id];
    justifier.x.fallback = &virtualPorts[id].mouse.x;
    justifier.y.fallback = &virtualPorts[id].mouse.y;
    justifier.trigger.fallback = &virtualPorts[id].mouse.left;
    justifier.start.fallback = &virtualPorts[id].mouse.right;

    auto& superScope = superScopeMappings[id];
    superScope.x.fallback = &virtualPorts[id].mouse.x;
    superScope.y.fallback = &virtualPorts[id].mouse.y;
    superScope.trigger.fallback = &virtualPorts[id].mouse.left;
    superScope.cursor.fallback = &virtualPorts[id].mouse.middle;
    superScope.turbo.fallback = &virtualPorts[id].mouse.right;
    superScope.pause.fallback = &virtualPorts[id].pad.start;

    auto& keypad = keypadMappings[id];
    keypad.up.fallback = &virtualPorts[id].pad.up;
    keypad.down.fallback = &virtualPorts[id].pad.down;
    keypad.left.fallback = &virtualPorts[id].pad.left;
    keypad.right.fallback = &virtualPorts[id].pad.right;
    keypad.b.fallback = &virtualPorts[id].pad.south;
    keypad.a.fallback = &virtualPorts[id].pad.east;
    keypad.y.fallback = &virtualPorts[id].pad.west;
    keypad.x.fallback = &virtualPorts[id].pad.north;
    keypad.l.fallback = &virtualPorts[id].pad.l_bumper;
    keypad.r.fallback = &virtualPorts[id].pad.r_bumper;
    keypad.back.fallback = &virtualPorts[id].pad.select;
    keypad.next.fallback = &virtualPorts[id].pad.start;
    keypad.one.fallback = &virtualPorts[id].pad.one;
    keypad.two.fallback = &virtualPorts[id].pad.two;
    keypad.three.fallback = &virtualPorts[id].pad.three;
    keypad.four.fallback = &virtualPorts[id].pad.four;
    keypad.five.fallback = &virtualPorts[id].pad.five;
    keypad.six.fallback = &virtualPorts[id].pad.six;
    keypad.seven.fallback = &virtualPorts[id].pad.seven;
    keypad.eight.fallback = &virtualPorts[id].pad.eight;
    keypad.nine.fallback = &virtualPorts[id].pad.nine;
    keypad.zero.fallback = &virtualPorts[id].pad.zero;
    keypad.star.fallback = &virtualPorts[id].pad.star;
    keypad.clear.fallback = &virtualPorts[id].pad.clear;
    keypad.pound.fallback = &virtualPorts[id].pad.pound;
    keypad.point.fallback = &virtualPorts[id].pad.point;
    keypad.end.fallback = &virtualPorts[id].pad.end;

    auto& twinTap = twinTapMappings[id];
    twinTap.one.fallback = &virtualPorts[id].pad.south;
    twinTap.two.fallback = &virtualPorts[id].pad.east;

    InputPort port{string{"Controller Port ", 1 + id}};

  { InputDevice device{"Gamepad"};
    device.digital("Up",     gamepad.up);
    device.digital("Down",   gamepad.down);
    device.digital("Left",   gamepad.left);
    device.digital("Right",  gamepad.right);
    device.digital("B",      gamepad.b);
    device.digital("A",      gamepad.a);
    device.digital("Y",      gamepad.y);
    device.digital("X",      gamepad.x);
    device.digital("L",      gamepad.l);
    device.digital("R",      gamepad.r);
    device.digital("Select", gamepad.select);
    device.digital("Start",  gamepad.start);
    port.append(device); }

  { InputDevice device{"Rumble Gamepad"};
    device.digital("Up",     rumbleGamepad.up);
    device.digital("Down",   rumbleGamepad.down);
    device.digital("Left",   rumbleGamepad.left);
    device.digital("Right",  rumbleGamepad.right);
    device.digital("B",      rumbleGamepad.b);
    device.digital("A",      rumbleGamepad.a);
    device.digital("Y",      rumbleGamepad.y);
    device.digital("X",      rumbleGamepad.x);
    device.digital("L",      rumbleGamepad.l);
    device.digital("R",      rumbleGamepad.r);
    device.digital("Select", rumbleGamepad.select);
    device.digital("Start",  rumbleGamepad.start);
    device.rumble("Rumble",  rumbleGamepad.rumble);
    port.append(device); }

  { InputDevice device{"Justifier"};
    device.relative("X",       justifier.x);
    device.relative("Y",       justifier.y);
    device.digital ("Trigger", justifier.trigger);
    device.digital ("Start",   justifier.start);
    port.append(device); }

  { InputDevice device{"Mouse"};
    device.relative("X",     mouse.x);
    device.relative("Y",     mouse.y);
    device.digital ("Left",  mouse.left);
    device.digital ("Right", mouse.right);
    port.append(device); }

  { InputDevice device{"NTT Data Keypad"};
    device.digital("Up", keypad.up);
    device.digital("Down", keypad.down);
    device.digital("Left", keypad.left);
    device.digital("Right", keypad.right);
    device.digital("B", keypad.b);
    device.digital("A", keypad.a);
    device.digital("Y", keypad.y);
    device.digital("X", keypad.x);
    device.digital("L", keypad.l);
    device.digital("R", keypad.r);
    device.digital("Back", keypad.back);
    device.digital("Next", keypad.next);
    device.digital("1", keypad.one);
    device.digital("2", keypad.two);
    device.digital("3", keypad.three);
    device.digital("4", keypad.four);
    device.digital("5", keypad.five);
    device.digital("6", keypad.six);
    device.digital("7", keypad.seven);
    device.digital("8", keypad.eight);
    device.digital("9", keypad.nine);
    device.digital("0", keypad.zero);
    device.digital("*", keypad.star);
    device.digital("C", keypad.clear);
    device.digital("#", keypad.pound);
    device.digital(".", keypad.point);
    device.digital("End", keypad.end);
    port.append(device); }

  { InputDevice device{"Super Scope"};
    device.relative("X",       superScope.x);
    device.relative("Y",       superScope.y);
    device.digital ("Trigger", superScope.trigger);
    device.digital ("Cursor",  superScope.cursor);
    device.digital ("Turbo",   superScope.turbo);
    device.digital ("Pause",   superScope.pause);
    port.append(device); }

  { InputDevice device{"Twin Tap"};
    device.digital("1", twinTap.one);
    device.digital("2", twinTap.two);
    port.append(device); }

    ports.push_back(port);
  }

  inputBlacklist = {"Justifiers", "Super Multitap"};
}

auto SuperFamicom::load() -> LoadResult {
  game = mia::Medium::create("Super Famicom");
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;

  system = mia::System::create("Super Famicom");
  result = system->load();
  if(result != successful) return result;

  ares::SuperFamicom::option("Pixel Accuracy", settings.video.pixelAccuracy);

  auto region = Emulator::region();
  if(!ares::SuperFamicom::load(root, {"[Nintendo] Super Famicom (", region, ")"})) return otherError;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    auto cartridge = port->allocate();
    port->connect();

    if(auto slot = cartridge->find<ares::Node::Port>("Super Game Boy/Cartridge Slot")) {
      gb = mia::Medium::create("Game Boy Color");
      if(gb->load(Emulator::load(gb, settings.paths.superFamicom.gameBoy)) == successful) {
        slot->allocate();
        slot->connect();
      } else {
        gb.reset();
      }
    }

    if(auto slot = cartridge->find<ares::Node::Port>("BS Memory Slot")) {
      bs = mia::Medium::create("BS Memory");
      if(bs->load(Emulator::load(bs, settings.paths.superFamicom.bsMemory)) == successful) {
        slot->allocate();
        slot->connect();
      } else {
        bs.reset();
      }
    }

    if(auto slot = cartridge->find<ares::Node::Port>("Sufami Turbo Slot A")) {
      stA = mia::Medium::create("Sufami Turbo");
      if(stA->load(Emulator::load(stA, settings.paths.superFamicom.sufamiTurbo)) == successful) {
        slot->allocate();
        slot->connect();
      } else {
        stA.reset();
      }
    }

    if(auto slot = cartridge->find<ares::Node::Port>("Sufami Turbo Slot B")) {
      stB = mia::Medium::create("Sufami Turbo");
      if(stB->load(Emulator::load(stB, settings.paths.superFamicom.sufamiTurbo)) == successful) {
        slot->allocate();
        slot->connect();
      } else {
        stB.reset();
      }
    }
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    port->allocate("Gamepad");
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 2")) {
    port->allocate("Gamepad");
    port->connect();
  }

  return successful;
}

auto SuperFamicom::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  if(gb) gb->save(gb->location);
  if(bs) bs->save(bs->location);
  if(stA) stA->save(stA->location);
  if(stB) stB->save(stB->location);
  return true;
}

auto SuperFamicom::pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> {
  if(node->name() == "Super Famicom") return system->pak;
  if(node->name() == "Super Famicom Cartridge") return game->pak;
  if(node->name() == "Game Boy Cartridge") return gb->pak;
  if(node->name() == "Game Boy Color Cartridge") return gb->pak;
  if(node->name() == "BS Memory Cartridge") return bs->pak;
  if(node->name() == "Sufami Turbo Cartridge") {
    auto wp = node->parent();
    if(!wp.expired()) {
      if(auto port = wp.lock()) {
        if(port->name() == "Sufami Turbo Slot A") return stA->pak;
        if(port->name() == "Sufami Turbo Slot B") return stB->pak;
      }
    }
  }
  return {};
}
