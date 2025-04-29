struct Nintendo64 : Emulator {
  Nintendo64();
  auto load() -> LoadResult override;
  auto load(Menu) -> void override;
  auto portMenu(Menu& portMenu, ares::Node::Port port) -> void override;
  auto unload() -> void override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;

  shared_pointer<mia::Pak> disk;
  u32 regionID = 0;
  sTimer diskInsertTimer;
};

Nintendo64::Nintendo64() {
  manufacturer = "Nintendo";
  name = "Nintendo 64";

  for(auto id : range(4)) {
    InputPort port{string{"Controller Port ", 1 + id}};

  { InputDevice device{"Gamepad"};
    device.analog ("L-Up",    virtualPorts[id].pad.lstick_up);
    device.analog ("L-Down",  virtualPorts[id].pad.lstick_down);
    device.analog ("L-Left",  virtualPorts[id].pad.lstick_left);
    device.analog ("L-Right", virtualPorts[id].pad.lstick_right);
    device.digital("Up",      virtualPorts[id].pad.up);
    device.digital("Down",    virtualPorts[id].pad.down);
    device.digital("Left",    virtualPorts[id].pad.left);
    device.digital("Right",   virtualPorts[id].pad.right);
    device.digital("B",       virtualPorts[id].pad.west);
    device.digital("A",       virtualPorts[id].pad.south);
    device.digital("C-Up",    virtualPorts[id].pad.rstick_up);
    device.digital("C-Down",  virtualPorts[id].pad.rstick_down);
    device.digital("C-Left",  virtualPorts[id].pad.rstick_left);
    device.digital("C-Right", virtualPorts[id].pad.rstick_right);
    device.digital("L",       virtualPorts[id].pad.l_bumper);
    device.digital("R",       virtualPorts[id].pad.r_bumper);
    device.digital("Z",       virtualPorts[id].pad.r_trigger);
    device.digital("Start",   virtualPorts[id].pad.start);
    device.rumble ("Rumble",  virtualPorts[id].pad.rumble);
    device.analog ("X-Axis",  virtualPorts[id].pad.lstick_left, virtualPorts[id].pad.lstick_right);
    device.analog ("Y-Axis",  virtualPorts[id].pad.lstick_up,   virtualPorts[id].pad.lstick_down);
    port.append(device); }

  { InputDevice device{"Mouse"};
    device.relative("X",     virtualPorts[id].mouse.x);
    device.relative("Y",     virtualPorts[id].mouse.y);
    device.digital ("Left",  virtualPorts[id].mouse.left);
    device.digital ("Right", virtualPorts[id].mouse.right);
    port.append(device); }
  
    ports.append(port);
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
  if(game->pak->attribute("dd").boolean()) {
    //use 64DD firmware settings
    vector<Firmware> firmware;
    for(auto& emulator : emulators) {
      if(emulator->name == "Nintendo 64DD") firmware = emulator->firmware;
    }
    if(!firmware) return otherError;  //should never occur
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
        if(id == 0 && game->pak->attribute("tpak").boolean()) {
          #if defined(CORE_GB)
          auto transferPak = port->allocate("Transfer Pak");
          port->connect();

          if(auto slot = transferPak->find<ares::Node::Port>("Cartridge Slot")) {
            gb = mia::Medium::create("Game Boy");
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
          if(id == 0 && game->pak->attribute("cpak").boolean()) {
            gamepad = mia::Pak::create("Nintendo 64");

            //create maximum sized controller pak, file is resized later
            gamepad->pak->append("save.pak", 1984_KiB);
            gamepad->load("save.pak", ".pak", game->location);
            port->allocate("Controller Pak");
            port->connect();
          } else if(game->pak->attribute("rpak").boolean()) {
            port->allocate("Rumble Pak");
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
      save();
      auto drive = root->find<ares::Node::Port>("Nintendo 64DD/Disk Drive");
      drive->disconnect();

      if(disk->load(Emulator::load(disk, configuration.game)) != successful) {
        return;
      }

      //give the emulator core a few seconds to notice an empty drive state before reconnecting
      diskInsertTimer->onActivate([&] {
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

  const string portNum = port->name()[port->name().length() - 1];

  // remove this check to enable pak menu option for all 4 controllers
  if(portNum == "1") {

    if(portMenu.actionCount() > 0) portMenu.append(MenuSeparator());
    Menu pakMenu{&portMenu};
    pakMenu.setText("Pak");
    Group pakGroup;
    MenuRadioItem nothing{&pakMenu};;
    nothing.setText("Nothing");
    nothing.setAttribute<ares::Node::Port>("port", port);
    nothing.onActivate([=] {
      auto port = nothing.attribute<ares::Node::Port>("port");
      const string portName = port->name();
      if(auto port = emulator->root->find<ares::Node::Port>(portName)) {
        port->disconnect();
        auto peripheral = port->allocate("Gamepad");
        port->connect();
      }
    });
    pakGroup.append(nothing);

    MenuRadioItem cpak{&pakMenu};
    cpak.setAttribute<ares::Node::Port>("port", port);
    cpak.setText("Controller Pak");
    cpak.onActivate([=] {
      auto port = cpak.attribute<ares::Node::Port>("port");
      const string portName = port->name();
      if(auto port = emulator->root->find<ares::Node::Port>(portName)) {
        port->disconnect();
        auto peripheral = port->allocate("Gamepad");
        port->connect();
        if(auto port = peripheral->find<ares::Node::Port>("Pak")) {
          emulator->gamepad = mia::Pak::create("Nintendo 64");
          emulator->gamepad->pak->append("save.pak", 32_KiB);
          string pakExt = ".pak";
          if(portNum != "1") { pakExt = string(".", portNum, ".pak");}
          emulator->gamepad->load("save.pak", pakExt, emulator->game->location);
          port->allocate("Controller Pak");
          port->connect();
        }
      }
    });
    pakGroup.append(cpak);

    MenuRadioItem rpak{&pakMenu};
    rpak.setAttribute<ares::Node::Port>("port", port);
    rpak.setText("Rumble Pak");
    rpak.onActivate([=] {
      auto port = rpak.attribute<ares::Node::Port>("port");
      const string portName = port->name();
      if(auto port = emulator->root->find<ares::Node::Port>(portName)) {
        port->disconnect();
        auto peripheral = port->allocate("Gamepad");
        port->connect();
        if(auto port = peripheral->find<ares::Node::Port>("Pak")) {
          port->allocate("Rumble Pak");
          port->connect();
        }
      }
    });
    pakGroup.append(rpak);

    MenuRadioItem tpak{&pakMenu};
    tpak.setAttribute<ares::Node::Port>("port", port);
    tpak.setText("Transfer Pak");
    tpak.onActivate([=] {
      auto port = tpak.attribute<ares::Node::Port>("port");
      const string portName = port->name();
      if(auto port = emulator->root->find<ares::Node::Port>(portName)) {
        port->disconnect();
        auto peripheral = port->allocate("Gamepad");
        port->connect();
        if(auto port = peripheral->find<ares::Node::Port>("Pak")) {
#if defined(CORE_GB)
          emulator->gb.reset();
          auto transferPak = port->allocate("Transfer Pak");
          port->connect();

          if(auto slot = transferPak->find<ares::Node::Port>("Cartridge Slot")) {
            emulator->gb = mia::Medium::create("Game Boy");
            string tmpPath;
            if(emulator->gb->load(emulator->load(emulator->gb, tmpPath)) == successful) {
              slot->allocate();
              slot->connect();
            } else {
              port->disconnect();
              emulator->gb.reset();
            }
          }
#endif
        }
      }
    });
    pakGroup.append(tpak);

    // set currently enabled pak
    // Note: based on initialization routine in desktop-ui/emulator/nintendo-64.cpp & nintedo-64dd.cpp
    if(emulator->game->pak->attribute("tpak").boolean() && portNum == "1") tpak.setChecked();
    else if(emulator->game->pak->attribute("cpak").boolean() && portNum == "1") cpak.setChecked();
    else if(emulator->game->pak->attribute("rpak").boolean()) rpak.setChecked();
    else nothing.setChecked();
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

auto Nintendo64::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Nintendo 64") return system->pak;
  if(node->name() == "Nintendo 64 Cartridge") return game->pak;
  if(node->name() == "Nintendo 64DD Disk" && disk) return disk->pak;
  if(node->name() == "Game Boy Cartridge") return gb->pak;
  if(node->name() == "Game Boy Color Cartridge") return gb->pak;
  if(node->name() == "Gamepad") return gamepad->pak;
  return {};
}
