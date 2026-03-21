struct Nintendo64 : Emulator {
  Nintendo64();
  auto load() -> LoadResult override;
  auto load(Menu) -> void override;
  auto portMenu(Menu& portMenu, ares::Node::Port port) -> void override;
  auto unload() -> void override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> std::shared_ptr<vfs::directory> override;

  struct GamepadMappings {
    InputAnalog lstickUp;
    InputAnalog lstickDown;
    InputAnalog lstickLeft;
    InputAnalog lstickRight;
    InputDigital up;
    InputDigital down;
    InputDigital left;
    InputDigital right;
    InputDigital b;
    InputDigital a;
    InputDigital cUp;
    InputDigital cDown;
    InputDigital cLeft;
    InputDigital cRight;
    InputDigital l;
    InputDigital r;
    InputDigital z;
    InputDigital start;
    InputRumble rumble;
  };

  struct MouseMappings {
    InputRelative x;
    InputRelative y;
    InputDigital left;
    InputDigital right;
  };

  GamepadMappings gamepadMappings[4];
  MouseMappings mouseMappings[4];
  std::shared_ptr<mia::Pak> disk;
  u32 regionID = 0;
  sTimer diskInsertTimer;
};

Nintendo64::Nintendo64() {
  manufacturer = "Nintendo";
  name = "Nintendo 64";

  for(auto id : range(4)) {
    auto& gamepad = gamepadMappings[id];
    gamepad.lstickUp.fallback = &virtualPorts[id].pad.lstick_up;
    gamepad.lstickDown.fallback = &virtualPorts[id].pad.lstick_down;
    gamepad.lstickLeft.fallback = &virtualPorts[id].pad.lstick_left;
    gamepad.lstickRight.fallback = &virtualPorts[id].pad.lstick_right;
    gamepad.up.fallback = &virtualPorts[id].pad.up;
    gamepad.down.fallback = &virtualPorts[id].pad.down;
    gamepad.left.fallback = &virtualPorts[id].pad.left;
    gamepad.right.fallback = &virtualPorts[id].pad.right;
    gamepad.b.fallback = &virtualPorts[id].pad.west;
    gamepad.a.fallback = &virtualPorts[id].pad.south;
    gamepad.cUp.fallback = &virtualPorts[id].pad.rstick_up;
    gamepad.cDown.fallback = &virtualPorts[id].pad.rstick_down;
    gamepad.cLeft.fallback = &virtualPorts[id].pad.rstick_left;
    gamepad.cRight.fallback = &virtualPorts[id].pad.rstick_right;
    gamepad.l.fallback = &virtualPorts[id].pad.l_bumper;
    gamepad.r.fallback = &virtualPorts[id].pad.r_bumper;
    gamepad.z.fallback = &virtualPorts[id].pad.r_trigger;
    gamepad.start.fallback = &virtualPorts[id].pad.start;
    gamepad.rumble.fallback = &virtualPorts[id].pad.rumble;

    auto& mouse = mouseMappings[id];
    mouse.x.fallback = &virtualPorts[id].mouse.x;
    mouse.y.fallback = &virtualPorts[id].mouse.y;
    mouse.left.fallback = &virtualPorts[id].mouse.left;
    mouse.right.fallback = &virtualPorts[id].mouse.right;

    InputPort port{string{"Controller Port ", 1 + id}};

  { InputDevice device{"Gamepad"};
    device.analog ("L-Up",    gamepad.lstickUp);
    device.analog ("L-Down",  gamepad.lstickDown);
    device.analog ("L-Left",  gamepad.lstickLeft);
    device.analog ("L-Right", gamepad.lstickRight);
    device.digital("Up",      gamepad.up);
    device.digital("Down",    gamepad.down);
    device.digital("Left",    gamepad.left);
    device.digital("Right",   gamepad.right);
    device.digital("B",       gamepad.b);
    device.digital("A",       gamepad.a);
    device.digital("C-Up",    gamepad.cUp);
    device.digital("C-Down",  gamepad.cDown);
    device.digital("C-Left",  gamepad.cLeft);
    device.digital("C-Right", gamepad.cRight);
    device.digital("L",       gamepad.l);
    device.digital("R",       gamepad.r);
    device.digital("Z",       gamepad.z);
    device.digital("Start",   gamepad.start);
    device.rumble ("Rumble",  gamepad.rumble);
    device.analog ("X-Axis",  gamepad.lstickLeft, gamepad.lstickRight);
    device.analog ("Y-Axis",  gamepad.lstickUp,   gamepad.lstickDown);
    port.append(device); }

  { InputDevice device{"Mouse"};
    device.relative("X",     mouse.x);
    device.relative("Y",     mouse.y);
    device.digital ("Left",  mouse.left);
    device.digital ("Right", mouse.right);
    port.append(device); }
  
    ports.push_back(port);
  }
}

auto Nintendo64::load() -> LoadResult {
  game = mia::Medium::create("Nintendo 64");
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;

  auto region = Emulator::region();

  string name;
  if(game->pak->attribute("dd").boolean() && !settings.general.noFilePrompt) {
    //use 64DD firmware settings
    std::vector<Firmware> firmware;
    for(auto& emulator : emulators) {
      if(emulator->name == "Nintendo 64DD") firmware = emulator->firmware;
    }
    if(firmware.empty()) return otherError;  //should never occur
    name = "Nintendo 64DD";

    disk = mia::Medium::create("Nintendo 64DD");
    if(disk->load(Emulator::load(disk, configuration.game)) != successful) {
      disk.reset();
      name = "Nintendo 64";
      system = mia::System::create("Nintendo 64");
      result = system->load();
      if(result != successful) return result;
    } else {
      region = disk->pak->attribute("region");
      //if statements below are ordered by lowest to highest priority
      if (region == "NTSC-DEV") regionID = 2;
      if (region == "NTSC-U") regionID = 1;
      if (region == "NTSC-J") regionID = 0;

      system = mia::System::create(name);
      result = system->load(firmware[regionID].location);
      if(result != successful) {
        result.firmwareSystemName = "Nintendo 64";
        result.firmwareType = firmware[regionID].type;
        result.firmwareRegion = firmware[regionID].region;
        result.result = noFirmware;
        return result;
      }
    }
  } else {
    name = "Nintendo 64";
    system = mia::System::create("Nintendo 64");
    result = system->load();
    if(result != successful) return result;
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
  ares::Nintendo64::option("Expansion Pak", settings.nintendo64.expansionPak);
  ares::Nintendo64::option("Controller Pak Banks", settings.nintendo64.controllerPakBankString);

  if(!ares::Nintendo64::load(root, {"[Nintendo] ", name, " (", region, ")"})) return otherError;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Nintendo 64DD/Disk Drive")) {
    port->allocate();
    port->connect();
  }

  auto controllers = 4;
  //Jeopardy! does not accept any input if > 3 controllers are plugged in at boot.
  if(game->pak->attribute("id") == "NJOE") controllers = min(controllers, 3);

  for(auto id : range(controllers)) {
    if(auto port = root->find<ares::Node::Port>({"Controller Port ", 1 + id})) {
      auto peripheral = port->allocate("Gamepad");
      port->connect();
      bool transferPakConnected = false;
      if(auto port = peripheral->find<ares::Node::Port>("Pak")) {
        if(game->pak->attribute({"port", id+1, "/tpak"}).boolean() && !settings.general.noFilePrompt) {
          #if defined(CORE_GB)
          auto transferPak = port->allocate("Transfer Pak");
          port->connect();

          if(auto slot = transferPak->find<ares::Node::Port>("Cartridge Slot")) {
            gb = mia::Medium::create("Game Boy Color");
            string tmpPath;
            if(gb->load(Emulator::load(gb, tmpPath)) == successful) {
              slot->allocate();
              slot->connect();
              transferPakConnected = true;
            } else {
              port->disconnect();
              gb.reset();
            }
          }
          #endif
        }

        if(!transferPakConnected) {
          if(game->pak->attribute({"port", id+1, "/cpak"}).boolean()) {
            gamepad = mia::Pak::create("Nintendo 64");

            //create maximum sized controller pak, file is resized later
            gamepad->pak->append("save.pak", 1984_KiB);
            gamepad->load("save.pak", ".pak", game->location);
            port->allocate("Controller Pak");
            port->connect();
          } else if(game->pak->attribute({"port", id+1, "/rpak"}).boolean()) {
            port->allocate("Rumble Pak");
            port->connect();
          } else if(game->pak->attribute({"port", id+1, "/biosensor"}).boolean()) {
            port->allocate("Bio Sensor");
            port->connect();
          }
        }
      }
    }
  }

  diskInsertTimer = Timer{};

  return successful;
}

auto Nintendo64::load(Menu menu) -> void {
  if(disk) {
    MenuItem changeDisk{&menu};
    changeDisk.setIcon(Icon::Device::Optical);
    changeDisk.setText("Change Disk").onActivate([&] {
      Program::Guard guard;
      save();
      auto drive = root->find<ares::Node::Port>("Nintendo 64DD/Disk Drive");
      drive->disconnect();

      if(disk->load(Emulator::load(disk, configuration.game)) != successful) {
        return;
      }

      //give the emulator core a few seconds to notice an empty drive state before reconnecting
      diskInsertTimer->onActivate([&] {
        Program::Guard guard;
        diskInsertTimer->setEnabled(false);
        auto drive = root->find<ares::Node::Port>("Nintendo 64DD/Disk Drive");
        drive->allocate();
        drive->connect();
      }).setInterval(3000).setEnabled();
    });
  }
}

auto Nintendo64::portMenu(Menu& portMenu, ares::Node::Port port) -> void {
  if(port->type() != "Controller") return;
  auto peripheral = port->connected();
  // Only show Pak menu if a Gamepad is connected
  if(!peripheral || peripheral->name() != "Gamepad") return;
  
  // Check what pak is currently connected
  ares::Node::Peripheral pak = nullptr;
  if(auto pakPort = peripheral->find<ares::Node::Port>("Pak")) {
    pak = pakPort->connected();
  }

  const string portNum = port->name()[port->name().length() - 1];

  if(portMenu.actionCount() > 0) portMenu.append(MenuSeparator());
  Menu pakMenu{&portMenu};
  pakMenu.setText("Pak");
  Group pakGroup;

  // Initialize all menu items and add them all to the group for appropriate radio selection behavior
  MenuRadioItem nothing{&pakMenu}, cpak{&pakMenu}, rpak{&pakMenu}, tpak{&pakMenu}, biosensor{&pakMenu};
  pakGroup.append(nothing);
  pakGroup.append(cpak);
  pakGroup.append(rpak);
  pakGroup.append(tpak);
  pakGroup.append(biosensor);

  // Show Nothing option
  nothing.setText("Nothing");
  nothing.setAttribute<ares::Node::Port>("port", port);
  if(!pak) nothing.setChecked();
  nothing.onActivate([=] {
    Program::Guard guard;
    auto port = nothing.attribute<ares::Node::Port>("port");
    const string portName = port->name();
    if(auto port = emulator->root->find<ares::Node::Port>(portName)) {
      if(auto peripheral = port->connected()) {
        if(auto pakPort = peripheral->find<ares::Node::Port>("Pak")) {
          pakPort->disconnect();
        }
      }
    }
    presentation.refreshSystemMenu();
  });

  // Show Controller Pak option
  cpak.setAttribute<ares::Node::Port>("port", port);
  cpak.setText("Controller Pak");
  if(pak && pak->name() == "Controller Pak") cpak.setChecked();
  cpak.onActivate([=] {
    Program::Guard guard;
    auto port = cpak.attribute<ares::Node::Port>("port");
    const string portName = port->name();
    if(auto port = emulator->root->find<ares::Node::Port>(portName)) {
      if(auto peripheral = port->connected()) {
        if(auto pakPort = peripheral->find<ares::Node::Port>("Pak")) {
          pakPort->disconnect();
          emulator->gamepad = mia::Pak::create("Nintendo 64");
          emulator->gamepad->pak->append("save.pak", 32_KiB);
          string pakExt = ".pak";
          if(portNum != "1") { pakExt = string(".", portNum, ".pak");}
          emulator->gamepad->load("save.pak", pakExt, emulator->game->location);
          pakPort->allocate("Controller Pak");
          pakPort->connect();
        }
      }
    }
    presentation.refreshSystemMenu();
  });

  // Show Rumble Pak option
  rpak.setAttribute<ares::Node::Port>("port", port);
  rpak.setText("Rumble Pak");
  if(pak && pak->name() == "Rumble Pak") rpak.setChecked();
  rpak.onActivate([=] {
    Program::Guard guard;
    auto port = rpak.attribute<ares::Node::Port>("port");
    const string portName = port->name();
    if(auto port = emulator->root->find<ares::Node::Port>(portName)) {
      if(auto peripheral = port->connected()) {
        if(auto pakPort = peripheral->find<ares::Node::Port>("Pak")) {
          pakPort->disconnect();
          pakPort->allocate("Rumble Pak");
          pakPort->connect();
        }
      }
    }
    presentation.refreshSystemMenu();
  });

  // Show Transfer Pak option if Game Boy core is available
#if defined(CORE_GB)
  tpak.setAttribute<ares::Node::Port>("port", port);
  tpak.setText("Transfer Pak");
  if(pak && pak->name() == "Transfer Pak") tpak.setChecked();
  tpak.onActivate([=] {
    Program::Guard guard;
    auto port = tpak.attribute<ares::Node::Port>("port");
    const string portName = port->name();
    if(auto port = emulator->root->find<ares::Node::Port>(portName)) {
      if(auto peripheral = port->connected()) {
        if(auto pakPort = peripheral->find<ares::Node::Port>("Pak")) {
          pakPort->disconnect();
          emulator->gb.reset();
          auto transferPak = pakPort->allocate("Transfer Pak");
          pakPort->connect();

          if(auto slot = transferPak->find<ares::Node::Port>("Cartridge Slot")) {
            emulator->gb = mia::Medium::create("Game Boy Color");
            string tmpPath;
            if(emulator->gb->load(emulator->load(emulator->gb, tmpPath)) == successful) {
              slot->allocate();
              slot->connect();
            } else {
              pakPort->disconnect();
              emulator->gb.reset();
            }
          }
        }
      }
    }
    presentation.refreshSystemMenu();
  });
#endif

  // Show Bio Sensor option
  biosensor.setAttribute<ares::Node::Port>("port", port);
  biosensor.setText("Bio Sensor");
  if(pak && pak->name() == "Bio Sensor") biosensor.setChecked();
  biosensor.onActivate([=] {
    Program::Guard guard;
    auto port = biosensor.attribute<ares::Node::Port>("port");
    const string portName = port->name();
    if(auto port = emulator->root->find<ares::Node::Port>(portName)) {
      if(auto peripheral = port->connected()) {
        if(auto pakPort = peripheral->find<ares::Node::Port>("Pak")) {
          pakPort->disconnect();
          pakPort->allocate("Bio Sensor");
          pakPort->connect();
        }
      }
    }
    presentation.refreshSystemMenu();
  });
  
  // Show Bio Sensor BPM menu if Bio Sensor is connected
  if(pak && pak->name() == "Bio Sensor") {
    // Find BPM setting directly from the Bio Sensor pak
    if(auto bpmSetting = pak->find<ares::Node::Setting::Integer>("Bio Sensor BPM")) {
      portMenu.append(MenuSeparator());
      Menu bpmMenu{&portMenu};
      bpmMenu.setText(bpmSetting->name());
      
      Group bpmGroup;
      for(auto value : bpmSetting->readAllowedValues()) {
        MenuRadioItem item{&bpmMenu};
        item.setText({value, " BPM"});
        s64 bpmValue = value.integer();
        if(bpmSetting->value() == bpmValue) item.setChecked();
        item.onActivate([bpmSetting, bpmValue]() mutable {
          bpmSetting->setValue(bpmValue);
        });
        bpmGroup.append(item);
      }
    }
  }
}

auto Nintendo64::unload() -> void {
  Emulator::unload();

  gamepad.reset();
  disk.reset();
  gb.reset();

  diskInsertTimer.reset();
}

auto Nintendo64::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  if(disk) disk->save(disk->location);
  if(gamepad) gamepad->save("save.pak", ".pak", game->location);
  if(gb) gb->save(gb->location);
  return true;
}

auto Nintendo64::pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> {
  if(node->name() == "Nintendo 64") return system->pak;
  if(node->name() == "Nintendo 64 Cartridge") return game->pak;
  if(node->name() == "Nintendo 64DD Disk" && disk) return disk->pak;
  if(node->name() == "Game Boy Cartridge") return gb->pak;
  if(node->name() == "Game Boy Color Cartridge") return gb->pak;
  if(node->name() == "Gamepad") return gamepad->pak;
  return {};
}
