struct SuperGrafxCD : Emulator {
  SuperGrafxCD();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;

  shared_pointer<mia::Pak> bios;
  u32 regionID = 0;
};

SuperGrafxCD::SuperGrafxCD() {
  manufacturer = "NEC";
  name = "SuperGrafx CD";

  firmware.append({"BIOS", "Japan"});  //NTSC-J

  for(auto id : range(5)) {
   InputPort port{string{"Controller Port ", 1 + id}};

  { InputDevice device{"Gamepad"};
    device.digital("Up",     virtualPorts[id].pad.up);
    device.digital("Down",   virtualPorts[id].pad.down);
    device.digital("Left",   virtualPorts[id].pad.left);
    device.digital("Right",  virtualPorts[id].pad.right);
    device.digital("II",     virtualPorts[id].pad.south);
    device.digital("I",      virtualPorts[id].pad.east);
    device.digital("Select", virtualPorts[id].pad.select);
    device.digital("Run",    virtualPorts[id].pad.start);
    port.append(device); }

    { InputDevice device{"Avenue Pad 6"};
    device.digital("Up",    virtualPorts[id].pad.up);
    device.digital("Down",  virtualPorts[id].pad.down);
    device.digital("Left",  virtualPorts[id].pad.left);
    device.digital("Right", virtualPorts[id].pad.right);
    device.digital("III",   virtualPorts[id].pad.west);
    device.digital("II",    virtualPorts[id].pad.south);
    device.digital("I",     virtualPorts[id].pad.east);
    device.digital("IV",    virtualPorts[id].pad.l_bumper);
    device.digital("V",     virtualPorts[id].pad.north);
    device.digital("VI",    virtualPorts[id].pad.r_bumper);
    device.digital("Select",virtualPorts[id].pad.select);
    device.digital("Run",   virtualPorts[id].pad.start);
    port.append(device); }

    ports.append(port);
  }

  portBlacklist = {"Controller Port"};
  inputBlacklist = {"Multitap"};
}

auto SuperGrafxCD::load() -> bool {
  game = mia::Medium::create("PC Engine CD");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  bios = mia::Medium::create("PC Engine");
  if(!bios->load(firmware[0].location)) return errorFirmware(firmware[0]), false;

  system = mia::System::create("SuperGrafx");
  if(!system->load()) return false;

  ares::PCEngine::option("Pixel Accuracy", settings.video.pixelAccuracy);

  if(!ares::PCEngine::load(root, "[NEC] SuperGrafx (NTSC-J)")) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("PC Engine CD/Disc Tray")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port")) {
    port->allocate("Multitap");
    port->connect();
  }

  for(auto id : range(5)) {
    if(auto port = root->scan<ares::Node::Port>(string{"Controller Port ", 1 + id})) {
      port->allocate("Gamepad");
      port->connect();
    }
  }

  return true;
}

auto SuperGrafxCD::save() -> bool {
  root->save();
  system->save(game->location);
  bios->save(game->location);
  game->save(game->location);
  return true;
}

auto SuperGrafxCD::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "SuperGrafx") return system->pak;
  if(node->name() == "SuperGrafx Card") return bios->pak;
  if(node->name() == "PC Engine CD Disc") return game->pak;
  return {};
}
