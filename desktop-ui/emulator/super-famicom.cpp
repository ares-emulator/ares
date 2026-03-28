struct SuperFamicom : Emulator {
  SuperFamicom();
  auto load() -> LoadResult override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> std::shared_ptr<vfs::directory> override;

  struct GamepadMappings {
    Dpad dpad;
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
    Dpad dpad;
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
    Dpad dpad;
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
    link(gamepad.dpad.up, virtualPorts[id].pad.up);
    link(gamepad.dpad.down, virtualPorts[id].pad.down);
    link(gamepad.dpad.left, virtualPorts[id].pad.left);
    link(gamepad.dpad.right, virtualPorts[id].pad.right);
    link(gamepad.b, virtualPorts[id].pad.south);
    link(gamepad.a, virtualPorts[id].pad.east);
    link(gamepad.y, virtualPorts[id].pad.west);
    link(gamepad.x, virtualPorts[id].pad.north);
    link(gamepad.l, virtualPorts[id].pad.l_bumper);
    link(gamepad.r, virtualPorts[id].pad.r_bumper);
    link(gamepad.select, virtualPorts[id].pad.select);
    link(gamepad.start, virtualPorts[id].pad.start);

    auto& rumbleGamepad = rumbleGamepadMappings[id];
    link(rumbleGamepad.dpad.up, virtualPorts[id].pad.up);
    link(rumbleGamepad.dpad.down, virtualPorts[id].pad.down);
    link(rumbleGamepad.dpad.left, virtualPorts[id].pad.left);
    link(rumbleGamepad.dpad.right, virtualPorts[id].pad.right);
    link(rumbleGamepad.b, virtualPorts[id].pad.south);
    link(rumbleGamepad.a, virtualPorts[id].pad.east);
    link(rumbleGamepad.y, virtualPorts[id].pad.west);
    link(rumbleGamepad.x, virtualPorts[id].pad.north);
    link(rumbleGamepad.l, virtualPorts[id].pad.l_bumper);
    link(rumbleGamepad.r, virtualPorts[id].pad.r_bumper);
    link(rumbleGamepad.select, virtualPorts[id].pad.select);
    link(rumbleGamepad.start, virtualPorts[id].pad.start);
    link(rumbleGamepad.rumble, virtualPorts[id].pad.rumble);

    auto& mouse = mouseMappings[id];
    link(mouse.x, virtualPorts[id].mouse.x);
    link(mouse.y, virtualPorts[id].mouse.y);
    link(mouse.left, virtualPorts[id].mouse.left);
    link(mouse.right, virtualPorts[id].mouse.right);

    auto& justifier = justifierMappings[id];
    link(justifier.x, virtualPorts[id].mouse.x);
    link(justifier.y, virtualPorts[id].mouse.y);
    link(justifier.trigger, virtualPorts[id].mouse.left);
    link(justifier.start, virtualPorts[id].mouse.right);

    auto& superScope = superScopeMappings[id];
    link(superScope.x, virtualPorts[id].mouse.x);
    link(superScope.y, virtualPorts[id].mouse.y);
    link(superScope.trigger, virtualPorts[id].mouse.left);
    link(superScope.cursor, virtualPorts[id].mouse.middle);
    link(superScope.turbo, virtualPorts[id].mouse.right);
    link(superScope.pause, virtualPorts[id].pad.start);

    auto& keypad = keypadMappings[id];
    link(keypad.dpad.up, virtualPorts[id].pad.up);
    link(keypad.dpad.down, virtualPorts[id].pad.down);
    link(keypad.dpad.left, virtualPorts[id].pad.left);
    link(keypad.dpad.right, virtualPorts[id].pad.right);
    link(keypad.b, virtualPorts[id].pad.south);
    link(keypad.a, virtualPorts[id].pad.east);
    link(keypad.y, virtualPorts[id].pad.west);
    link(keypad.x, virtualPorts[id].pad.north);
    link(keypad.l, virtualPorts[id].pad.l_bumper);
    link(keypad.r, virtualPorts[id].pad.r_bumper);
    link(keypad.back, virtualPorts[id].pad.select);
    link(keypad.next, virtualPorts[id].pad.start);
    link(keypad.one, virtualPorts[id].pad.one);
    link(keypad.two, virtualPorts[id].pad.two);
    link(keypad.three, virtualPorts[id].pad.three);
    link(keypad.four, virtualPorts[id].pad.four);
    link(keypad.five, virtualPorts[id].pad.five);
    link(keypad.six, virtualPorts[id].pad.six);
    link(keypad.seven, virtualPorts[id].pad.seven);
    link(keypad.eight, virtualPorts[id].pad.eight);
    link(keypad.nine, virtualPorts[id].pad.nine);
    link(keypad.zero, virtualPorts[id].pad.zero);
    link(keypad.star, virtualPorts[id].pad.star);
    link(keypad.clear, virtualPorts[id].pad.clear);
    link(keypad.pound, virtualPorts[id].pad.pound);
    link(keypad.point, virtualPorts[id].pad.point);
    link(keypad.end, virtualPorts[id].pad.end);

    auto& twinTap = twinTapMappings[id];
    link(twinTap.one, virtualPorts[id].pad.south);
    link(twinTap.two, virtualPorts[id].pad.east);

    InputPort port{string{"Controller Port ", 1 + id}};

  { InputDevice device{"Gamepad"};
    device.digital("Up",     gamepad.dpad.up);
    device.digital("Down",   gamepad.dpad.down);
    device.digital("Left",   gamepad.dpad.left);
    device.digital("Right",  gamepad.dpad.right);
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
    device.digital("Up",     rumbleGamepad.dpad.up);
    device.digital("Down",   rumbleGamepad.dpad.down);
    device.digital("Left",   rumbleGamepad.dpad.left);
    device.digital("Right",  rumbleGamepad.dpad.right);
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
    device.digital("Up", keypad.dpad.up);
    device.digital("Down", keypad.dpad.down);
    device.digital("Left", keypad.dpad.left);
    device.digital("Right", keypad.dpad.right);
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
