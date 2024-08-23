struct MegaCD32X : Emulator {
  MegaCD32X();
  auto load() -> bool override;
  auto load(Menu) -> void override;
  auto unload() -> void override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;

  u32 regionID = 0;
  sTimer discTrayTimer;
};

MegaCD32X::MegaCD32X() {
  manufacturer = "Sega";
  name = "Mega CD 32X";

  for(auto id : range(2)) {
    InputPort port{string{"Controller Port ", 1 + id}};

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

  { InputDevice device{"Mega Mouse"};
    device.relative("X",      virtualPorts[id].mouse.x);
    device.relative("Y",      virtualPorts[id].mouse.y);
    device.digital ("Left",   virtualPorts[id].mouse.left);
    device.digital ("Right",  virtualPorts[id].mouse.right);
    device.digital ("Middle", virtualPorts[id].mouse.middle);
    device.digital ("Start",  virtualPorts[id].mouse.extra);
    port.append(device); }

    ports.append(port);
  }
}

auto MegaCD32X::load() -> bool {
  game = mia::Medium::create("Mega CD");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  auto region = Emulator::region();
  //if statements below are ordered by lowest to highest priority
  if(region == "PAL"   ) regionID = 2;
  if(region == "NTSC-J") regionID = 1;
  if(region == "NTSC-U") regionID = 0;

  //use Mega CD firmware settings
  vector<Firmware> firmware;
  for(auto& emulator : emulators) {
    if(emulator->name == "Mega CD") firmware = emulator->firmware;
  }
  if(!firmware) return false;  //should never occur

  system = mia::System::create("Mega CD 32X");
  if(!system->load(firmware[regionID].location)) return errorFirmware(firmware[regionID], "Mega CD"), false;

  ares::MegaDrive::option("Recompiler", !settings.general.forceInterpreter);

  if(!ares::MegaDrive::load(root, {"[Sega] Mega CD 32X (", region, ")"})) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Mega CD/Disc Tray")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    port->allocate("Fighting Pad");
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 2")) {
    if(game->pak->attribute("serial").beginsWith("GM T-16201F")) {
      // CORPSE KILLER 32X (U) -- GM T-16201F-00
      // CORPSE KILLER 32X (E) -- GM T-16201F-50
      // Gamepad in controller port 2 breaks input polling, so leave it disconnected.
      // No supported lightgun devices are currently emulated (Sega Menacer, ALG GameGun).
    } else {
      port->allocate("Fighting Pad");
      port->connect();
    }
  }

  discTrayTimer = Timer{};

  return true;
}


auto MegaCD32X::load(Menu menu) -> void {
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
    discTrayTimer->onActivate([&] {
      discTrayTimer->setEnabled(false);
      auto tray = root->find<ares::Node::Port>("Mega CD/Disc Tray");
      tray->allocate();
      tray->connect();
    }).setInterval(3000).setEnabled();
  });
}

auto MegaCD32X::unload() -> void {
  Emulator::unload();
  discTrayTimer.reset();
}

auto MegaCD32X::save() -> bool {
  root->save();
  system->save(game->location);
  game->save(game->location);
  return true;
}

auto MegaCD32X::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Mega Drive") return system->pak;
  if(node->name() == "Mega CD Disc") return game->pak;
  return {};
}
