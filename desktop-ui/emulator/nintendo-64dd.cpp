struct Nintendo64DD : Nintendo64 {
  Nintendo64DD();
  auto load() -> LoadResult override;
  auto load(Menu) -> void override;
  auto unload() -> void override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;

  u32 regionID = 0;
  sTimer diskInsertTimer;
};

Nintendo64DD::Nintendo64DD() {
  manufacturer = "Nintendo";
  name = "Nintendo 64DD";

  firmware.push_back({"BIOS", "Japan", "806400ec0df94b0755de6c5b8249d6b6a9866124c5ddbdac198bde22499bfb8b"});
  firmware.push_back({"BIOS", "US", "e9fec87a45fba02399e88064b9e2f8cf0f2106e351c58279a87f05da5bc984ad"});
  firmware.push_back({"BIOS", "DEV", "9c2962a8b994a29e4cd04b3a6e4ed730a751414655ab6a9799ebf5fc08b79d44"});

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

    ports.push_back(port);
  }
}

auto Nintendo64DD::load() -> LoadResult {
  game = mia::Medium::create("Nintendo 64DD");
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;

  auto region = game->pak->attribute("region");
  //if statements below are ordered by lowest to highest priority
  if (region == "NTSC-DEV") regionID = 2;
  if (region == "NTSC-U"  ) regionID = 1;
  if (region == "NTSC-J"  ) regionID = 0;

  system = mia::System::create("Nintendo 64DD");
  result = system->load(firmware[regionID].location);
  if(result != successful) {
    result.firmwareSystemName = "Nintendo 64DD";
    result.firmwareType = firmware[regionID].type;
    result.firmwareRegion = firmware[regionID].region;
    result.result = noFirmware;
    return result;
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

  if(!ares::Nintendo64::load(root, {"[Nintendo] Nintendo 64DD (", region, ")"})) return otherError;

  if(auto port = root->find<ares::Node::Port>("Nintendo 64DD/Disk Drive")) {
    port->allocate();
    port->connect();
  }

  auto controllers = 4;
  for(auto id : range(controllers)) {
    if(auto port = root->find<ares::Node::Port>({"Controller Port ", 1 + id})) {
      auto peripheral = port->allocate("Gamepad");
      port->connect();
      bool transferPakConnected = false;
      if(auto port = peripheral->find<ares::Node::Port>("Pak")) {
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
        if(id == 0 && game->pak->attribute("tpak").boolean()) {
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
          if(id == 0 && game->pak->attribute("cpak").boolean()) {
            gamepad = mia::Pak::create("Nintendo 64");
            gamepad->pak->append("save.pak", 32_KiB);
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

auto Nintendo64DD::load(Menu menu) -> void {
  MenuItem changeDisk{&menu};
  changeDisk.setIcon(Icon::Device::Optical);
  changeDisk.setText("Change Disk").onActivate([&] {
    Program::Guard guard;
    save();
    auto drive = root->find<ares::Node::Port>("Nintendo 64DD/Disk Drive");
    drive->disconnect();

    if(game->load(Emulator::load(game, configuration.game)) != successful) {
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

auto Nintendo64DD::unload() -> void {
  gamepad.reset();

  Emulator::unload();
  diskInsertTimer.reset();
}

auto Nintendo64DD::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  if(gamepad) gamepad->save("save.pak", ".pak", game->location);
  return true;
}

auto Nintendo64DD::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Nintendo 64") return system->pak;
  if(node->name() == "Nintendo 64DD Disk") return game->pak;
  if(node->name() == "Gamepad") return gamepad->pak;
  return {};
}
