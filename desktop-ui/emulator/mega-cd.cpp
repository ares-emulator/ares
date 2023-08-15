struct MegaCD : Emulator {
  MegaCD();
  auto load() -> bool override;
  auto load(Menu) -> void override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;

  u32 regionID = 0;
  Timer discTrayTimer;
};

MegaCD::MegaCD() {
  manufacturer = "Sega";
  name = "Mega CD";

  firmware.append({"BIOS", "US"});      //NTSC-U
  firmware.append({"BIOS", "Japan"});   //NTSC-J
  firmware.append({"BIOS", "Europe"});  //PAL

  for(auto id : range(2)) {
    InputPort port{string{"Controller Port ", 1 + id}};

  { InputDevice device{"Control Pad"};
    device.digital("Up",    virtualPorts[id].pad.up);
    device.digital("Down",  virtualPorts[id].pad.down);
    device.digital("Left",  virtualPorts[id].pad.left);
    device.digital("Right", virtualPorts[id].pad.right);
    device.digital("A",     virtualPorts[id].pad.west);
    device.digital("B",     virtualPorts[id].pad.south);
    device.digital("C",     virtualPorts[id].pad.east);
    device.digital("Start", virtualPorts[id].pad.start);
    port.append(device); }

  { InputDevice device{"Fighting Pad"};
    device.digital("Up",    virtualPorts[id].pad.up);
    device.digital("Down",  virtualPorts[id].pad.down);
    device.digital("Left",  virtualPorts[id].pad.left);
    device.digital("Right", virtualPorts[id].pad.right);
    device.digital("A",     virtualPorts[id].pad.west);
    device.digital("B",     virtualPorts[id].pad.south);
    device.digital("C",     virtualPorts[id].pad.east);
    device.digital("X",     virtualPorts[id].pad.l_bumper);
    device.digital("Y",     virtualPorts[id].pad.north);
    device.digital("Z",     virtualPorts[id].pad.r_bumper);
    device.digital("Mode",  virtualPorts[id].pad.select);
    device.digital("Start", virtualPorts[id].pad.start);
    port.append(device); }

    ports.append(port);
  }
}

auto MegaCD::load() -> bool {
  game = mia::Medium::create("Mega CD");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  auto region = Emulator::region();
  //if statements below are ordered by lowest to highest priority
  if(region == "PAL"   ) regionID = 2;
  if(region == "NTSC-J") regionID = 1;
  if(region == "NTSC-U") regionID = 0;

  system = mia::System::create("Mega CD");
  if(!system->load(firmware[regionID].location)) return errorFirmware(firmware[regionID]), false;

  if(!ares::MegaDrive::load(root, {"[Sega] Mega CD (", region, ")"})) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Mega CD/Disc Tray")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    port->allocate("Control Pad");
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 2")) {
    if(game->pak->attribute("serial").beginsWith("GM T-162055")) {
      // CORPSE KILLER (U) -- GM T-162055-00
      // CORPSE KILLER (E) -- GM T-162055-50
      // Gamepad in controller port 2 breaks input polling, so leave it disconnected.
      // No supported lightgun devices are currently emulated (Sega Menacer, ALG GameGun).
    } else {
      port->allocate("Control Pad");
      port->connect();
    }
  }

  return true;
}

auto MegaCD::load(Menu menu) -> void {
  MenuItem changeDisc{&menu};
  changeDisc.setIcon(Icon::Device::Optical);
  changeDisc.setText("Change Disc").onActivate([&] {
    save();
    auto tray = root->find<ares::Node::Port>("Mega CD/Disc Tray");
    tray->disconnect();

    if(!game->load(Emulator::load(game, configuration.game))) {
      return;
    }

    //give the emulator core a few seconds to notice an empty drive state before reconnecting
    discTrayTimer.onActivate([&] {
      discTrayTimer.setEnabled(false);
      auto tray = root->find<ares::Node::Port>("Mega CD/Disc Tray");
      tray->allocate();
      tray->connect();
    }).setInterval(3000).setEnabled();
  });
}

auto MegaCD::save() -> bool {
  root->save();
  system->save(game->location);
  game->save(game->location);
  return true;
}

auto MegaCD::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Mega Drive") return system->pak;
  if(node->name() == "Mega CD Disc") return game->pak;
  return {};
}
