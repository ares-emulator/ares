struct MegaLD : Emulator {
  MegaLD();
  auto load() -> LoadResult override;
  auto load(Menu) -> void override;
  auto unload() -> void override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> std::shared_ptr<vfs::directory> override;
  auto changeDiskState(const string state) -> void;

  u32 regionID = 0;
  sTimer discTrayTimer;
};

MegaLD::MegaLD() {
  manufacturer = "Pioneer";
  name = "LaserActive (SEGA PAC)";

  firmware.push_back({"BIOS", "US"});      //NTSC-U
  firmware.push_back({"BIOS", "Japan"});   //NTSC-J

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

    ports.push_back(port);
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

  if(!ares::MegaDrive::load(root, {"[Pioneer] LaserActive (SEGA PAC) (", region, ")"})) return otherError;

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
  Group group;
  Menu changeSideMenu{&menu};
  changeSideMenu.setIcon(Icon::Device::Optical);
  changeSideMenu.setText("Change Side");
  auto medium = game->pak->attribute("medium");
  auto sides = nall::split_and_strip(medium, ",");

  MenuRadioItem noDiscItem{&changeSideMenu};
  noDiscItem.setText("No Disc").onActivate([&] {
    changeDiskState("No Disc");
  });
  group.append(noDiscItem);

  auto checkedItemIndex = (sides.size() > 0 ? group.objectCount() : 0);
  for(auto side : sides) {
    MenuRadioItem item{ &changeSideMenu };
    group.append(item);
    item.setText(side).onActivate([this, side] { changeDiskState(side); });
  }
  group.objects<MenuRadioItem>()[checkedItemIndex].setChecked();
}

auto MegaLD::changeDiskState(const string state) -> void {
  Program::Guard guard;
  discTrayTimer->setEnabled(false);
  save();
  auto tray = root->find<ares::Node::Port>("Mega CD/Disc Tray");
  tray->disconnect();

  if(state == "No Disc") return;

  discTrayTimer->onActivate([&, state] {
    Program::Guard guard;
    discTrayTimer->setEnabled(false);
    auto tray = root->find<ares::Node::Port>("Mega CD/Disc Tray");
    tray->allocate(state);
    tray->connect();
  }).setInterval(3000).setEnabled();
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

auto MegaLD::pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> {
  if(node->name() == "Mega Drive") return system->pak;
  if(node->name() == "Mega CD Disc") return game->pak;
  return {};
}
