struct MegaLD : Emulator {
  MegaLD();
  auto load() -> LoadResult override;
  auto load(Menu) -> void override;
  auto unload() -> void override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;

  u32 regionID = 0;
  sTimer discTrayTimer;
};

MegaLD::MegaLD() {
  manufacturer = "Sega";
  name = "Mega LD";

  firmware.append({"BIOS", "US"});      //NTSC-U
  firmware.append({"BIOS", "Japan"});   //NTSC-J

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

    ports.append(port);
  }
}

auto MegaLD::load() -> LoadResult {
  game = mia::Medium::create("Mega LD");
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;

  auto region = Emulator::region();
  //if statements below are ordered by lowest to highest priority
  if(region == "NTSC-J") regionID = 1;
  if(region == "NTSC-U") regionID = 0;

  system = mia::System::create("Mega LD");
  result = system->load(firmware[regionID].location);
  if(result != successful) {
    result.firmwareSystemName = "Mega LD";
    result.firmwareType = firmware[regionID].type;
    result.firmwareRegion = firmware[regionID].region;
    result.result = noFirmware;
    return result;
  }

  if(!ares::MegaDrive::load(root, {"[Sega] Mega LD (", region, ")"}, location)) return otherError;

  if(auto port = root->find<ares::Node::Port>("Mega CD/Disc Tray")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    port->allocate("Fighting Pad");
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 2")) {
    port->allocate("Fighting Pad");
    port->connect();
  }


  discTrayTimer = Timer{};
  return successful;
}

auto MegaLD::load(Menu menu) -> void {
  MenuItem changeDisc{&menu};
  changeDisc.setIcon(Icon::Device::Optical);
  changeDisc.setText("Change Disc").onActivate([&] {
    save();
    auto tray = root->find<ares::Node::Port>("Mega CD/Disc Tray");
    tray->disconnect();

    if(game->load(Emulator::load(game, configuration.game)) != successful) {
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

auto MegaLD::unload() -> void {
  Emulator::unload();
  discTrayTimer.reset();
}


auto MegaLD::save() -> bool {
  root->save();
  system->save(game->location);
  game->save(game->location);
  return true;
}

auto MegaLD::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Mega Drive") return system->pak;
  if(node->name() == "Mega CD Disc") return game->pak;
  return {};
}
