struct SuperFamicom : Emulator {
  SuperFamicom();
  auto load() -> LoadResult override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> std::shared_ptr<vfs::directory> override;

  std::shared_ptr<mia::Pak> gb, bs, stA, stB;
};

SuperFamicom::SuperFamicom() {
  manufacturer = "Nintendo";
  name = "Super Famicom";

  for(auto id : range(2)) {
    InputPort port{string{"Controller Port ", 1 + id}};

  { InputDevice device{"Gamepad"};
    device.digital("Up",     virtualPorts[id].pad.up);
    device.digital("Down",   virtualPorts[id].pad.down);
    device.digital("Left",   virtualPorts[id].pad.left);
    device.digital("Right",  virtualPorts[id].pad.right);
    device.digital("B",      virtualPorts[id].pad.south);
    device.digital("A",      virtualPorts[id].pad.east);
    device.digital("Y",      virtualPorts[id].pad.west);
    device.digital("X",      virtualPorts[id].pad.north);
    device.digital("L",      virtualPorts[id].pad.l_bumper);
    device.digital("R",      virtualPorts[id].pad.r_bumper);
    device.digital("Select", virtualPorts[id].pad.select);
    device.digital("Start",  virtualPorts[id].pad.start);
    port.append(device); }

  { InputDevice device{"Rumble Gamepad"};
    device.digital("Up",     virtualPorts[id].pad.up);
    device.digital("Down",   virtualPorts[id].pad.down);
    device.digital("Left",   virtualPorts[id].pad.left);
    device.digital("Right",  virtualPorts[id].pad.right);
    device.digital("B",      virtualPorts[id].pad.south);
    device.digital("A",      virtualPorts[id].pad.east);
    device.digital("Y",      virtualPorts[id].pad.west);
    device.digital("X",      virtualPorts[id].pad.north);
    device.digital("L",      virtualPorts[id].pad.l_bumper);
    device.digital("R",      virtualPorts[id].pad.r_bumper);
    device.digital("Select", virtualPorts[id].pad.select);
    device.digital("Start",  virtualPorts[id].pad.start);
    device.rumble("Rumble",  virtualPorts[id].pad.rumble);
    port.append(device); }

  { InputDevice device{"Justifier"};
    device.relative("X",       virtualPorts[id].mouse.x);
    device.relative("Y",       virtualPorts[id].mouse.y);
    device.digital ("Trigger", virtualPorts[id].mouse.left);
    device.digital ("Start",   virtualPorts[id].mouse.right);
    port.append(device); }

  { InputDevice device{"Mouse"};
    device.relative("X",     virtualPorts[id].mouse.x);
    device.relative("Y",     virtualPorts[id].mouse.y);
    device.digital ("Left",  virtualPorts[id].mouse.left);
    device.digital ("Right", virtualPorts[id].mouse.right);
    port.append(device); }

  { InputDevice device{"NTT Data Keypad"};
    device.digital("Up", virtualPorts[id].pad.up);
    device.digital("Down", virtualPorts[id].pad.down);
    device.digital("Left", virtualPorts[id].pad.left);
    device.digital("Right", virtualPorts[id].pad.right);
    device.digital("B", virtualPorts[id].pad.south);
    device.digital("A", virtualPorts[id].pad.east);
    device.digital("Y", virtualPorts[id].pad.west);
    device.digital("X", virtualPorts[id].pad.north);
    device.digital("L", virtualPorts[id].pad.l_bumper);
    device.digital("R", virtualPorts[id].pad.r_bumper);
    device.digital("Back", virtualPorts[id].pad.select);
    device.digital("Next", virtualPorts[id].pad.start);
    device.digital("1", virtualPorts[id].pad.one);
    device.digital("2", virtualPorts[id].pad.two);
    device.digital("3", virtualPorts[id].pad.three);
    device.digital("4", virtualPorts[id].pad.four);
    device.digital("5", virtualPorts[id].pad.five);
    device.digital("6", virtualPorts[id].pad.six);
    device.digital("7", virtualPorts[id].pad.seven);
    device.digital("8", virtualPorts[id].pad.eight);
    device.digital("9", virtualPorts[id].pad.nine);
    device.digital("0", virtualPorts[id].pad.zero);
    device.digital("*", virtualPorts[id].pad.star);
    device.digital("C", virtualPorts[id].pad.clear);
    device.digital("#", virtualPorts[id].pad.pound);
    device.digital(".", virtualPorts[id].pad.point);
    device.digital("End", virtualPorts[id].pad.end);
    port.append(device); }

  { InputDevice device{"Super Scope"};
    device.relative("X",       virtualPorts[id].mouse.x);
    device.relative("Y",       virtualPorts[id].mouse.y);
    device.digital ("Trigger", virtualPorts[id].mouse.left);
    device.digital ("Cursor",  virtualPorts[id].mouse.middle);
    device.digital ("Turbo",   virtualPorts[id].mouse.right);
    device.digital ("Pause",   virtualPorts[id].pad.start);
    port.append(device); }

  { InputDevice device{"Twin Tap"};
    device.digital("1", virtualPorts[id].pad.south);
    device.digital("2", virtualPorts[id].pad.east);
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
      gb = mia::Medium::create("Game Boy");
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
    if(auto parent = node->parent()) {
      if(auto port = parent.acquire()) {
        if(port->name() == "Sufami Turbo Slot A") return stA->pak;
        if(port->name() == "Sufami Turbo Slot B") return stB->pak;
      }
    }
  }
  return {};
}
