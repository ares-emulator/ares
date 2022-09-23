struct Nintendo64 : Emulator {
  Nintendo64();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;

  shared_pointer<mia::Pak> gamepad;
  shared_pointer<mia::Pak> disk;
  u32 regionID = 0;
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

auto Nintendo64::load() -> bool {
  game = mia::Medium::create("Nintendo 64");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  auto region = Emulator::region();

  string name;
  if(game->pak->attribute("dd").boolean()) {
    //use 64DD firmware settings
    vector<Firmware> firmware;
    for(auto& emulator : emulators) {
      if(emulator->name == "Nintendo 64DD") firmware = emulator->firmware;
    }
    if(!firmware) return false;  //should never occur
    name = "Nintendo 64DD";

    disk = mia::Medium::create("Nintendo 64DD");
    if(!disk->load(Emulator::load(disk, configuration.game))) {
      disk.reset();
      name = "Nintendo 64";
      system = mia::System::create("Nintendo 64");
      if(!system->load()) return false;
    } else {
      region = disk->pak->attribute("region");
      //if statements below are ordered by lowest to highest priority
      if (region == "NTSC-DEV") regionID = 2;
      if (region == "NTSC-U") regionID = 1;
      if (region == "NTSC-J") regionID = 0;

      system = mia::System::create(name);
      if(!system->load(firmware[regionID].location)) return errorFirmware(firmware[regionID], "Nintendo 64DD"), false;
    }
  } else {
    name = "Nintendo 64";
    system = mia::System::create("Nintendo 64");
    if(!system->load()) return false;
  }

  ares::Nintendo64::option("Quality", settings.video.quality);
  ares::Nintendo64::option("Supersampling", settings.video.supersampling);
  ares::Nintendo64::option("Enable GPU acceleration", settings.video.enableVulkan);
  ares::Nintendo64::option("Disable Video Interface Processing", settings.video.disableVideoInterfaceProcessing);

  if(!ares::Nintendo64::load(root, {"[Nintendo] ", name, " (", region, ")"})) return false;

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
      if(auto port = peripheral->find<ares::Node::Port>("Pak")) {
        if(id == 0 && game->pak->attribute("mempak").boolean()) {
          gamepad = mia::Pak::create("Nintendo 64");
          gamepad->pak->append("save.pak", 32_KiB);
          gamepad->load("save.pak", ".pak", game->location);
          port->allocate("Controller Pak");
          port->connect();
        } else if(game->pak->attribute("rumble").boolean()) {
          port->allocate("Rumble Pak");
          port->connect();
        }
      }
    }
  }

  return true;
}

auto Nintendo64::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  if(disk) disk->save(disk->location);
  if(gamepad) gamepad->save("save.pak", ".pak", game->location);
  return true;
}

auto Nintendo64::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Nintendo 64") return system->pak;
  if(node->name() == "Nintendo 64 Cartridge") return game->pak;
  if(node->name() == "Nintendo 64DD Disk" && disk) return disk->pak;
  if(node->name() == "Gamepad") return gamepad->pak;
  return {};
}
