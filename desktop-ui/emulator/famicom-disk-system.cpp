struct FamicomDiskSystem : Emulator {
  FamicomDiskSystem();
  auto load(Menu) -> void override;
  auto load() -> LoadResult override;
  auto unload() -> void override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> std::shared_ptr<vfs::directory> override;
  auto notify(const string& message) -> void override;
  auto changeDiskState(const string state) -> void;

  std::shared_ptr<mia::Pak> bios;
  sTimer diskChangeTimer;
};

FamicomDiskSystem::FamicomDiskSystem() {
  manufacturer = "Nintendo";
  name = "Famicom Disk System";

  firmware.push_back({"BIOS", "Japan", "fdc1a76e654feea993fcb38366e05ee5f4eb641f86fe6bebaeefd412e112dd72"});

  for(auto id : range(2)) {
    InputPort port{string{"Controller Port ", 1 + id}};

  { InputDevice device{"Gamepad"};
    device.digital("Up",         virtualPorts[id].pad.up);
    device.digital("Down",       virtualPorts[id].pad.down);
    device.digital("Left",       virtualPorts[id].pad.left);
    device.digital("Right",      virtualPorts[id].pad.right);
    device.digital("B",          virtualPorts[id].pad.west);
    device.digital("A",          virtualPorts[id].pad.south);
    device.digital("Select",     virtualPorts[id].pad.select);
    device.digital("Start",      virtualPorts[id].pad.start);
    device.digital("Microphone", virtualPorts[id].pad.north);
    port.append(device); }

    ports.push_back(port);
  }
}

auto FamicomDiskSystem::load(Menu menu) -> void {
  Group group;
  Menu diskMenu{&menu};
  diskMenu.setText("Disk Drive").setIcon(Icon::Media::Floppy);

  MenuRadioItem ejected{&diskMenu};
  ejected.setText("No Disk").onActivate([&] { changeDiskState("Ejected"); });
  group.append(ejected);
  if(game->pak->count() < 2) return (void)ejected.setChecked();

  MenuRadioItem disk1sideA{&diskMenu};
  disk1sideA.setText("Disk 1: Side A").onActivate([&] { changeDiskState("Disk 1: Side A"); });
  group.append(disk1sideA);
  if(game->pak->count() < 3) return (void)disk1sideA.setChecked();

  MenuRadioItem disk1sideB{&diskMenu};
  disk1sideB.setText("Disk 1: Side B").onActivate([&] { changeDiskState("Disk 1: Side B"); });
  group.append(disk1sideB);
  if(game->pak->count() < 4) return (void)disk1sideA.setChecked();

  MenuRadioItem disk2sideA{&diskMenu};
  disk2sideA.setText("Disk 2: Side A").onActivate([&] { changeDiskState("Disk 2: Side A"); });
  group.append(disk2sideA);
  if(game->pak->count() < 5) return (void)disk1sideA.setChecked();

  MenuRadioItem disk2sideB{&diskMenu};
  disk2sideB.setText("Disk 2: Side B").onActivate([&] { changeDiskState("Disk 2: Side B"); });
  group.append(disk2sideB);
  return (void)disk1sideA.setChecked();
}

auto FamicomDiskSystem::changeDiskState(string state) -> void
{
  Program::Guard guard;
  print("Changing disk state to: ", state, "\n");
  //Eject the disk and give the emulator time to react before re-inserting
  print("Ejecting disk\n");
  emulator->notify("Ejected");
  if(state != "Ejected") {
    print("Setting disk change timer\n");
    diskChangeTimer->onActivate([=, this] {
      Program::Guard guard;
      print("Disk change timer activated, setting disk state to: ", state, "\n");
      diskChangeTimer->setEnabled(false);
      emulator->notify(state);
    }).setInterval(3000).setEnabled();
  }
}

auto FamicomDiskSystem::load() -> LoadResult {
  game = std::dynamic_pointer_cast<mia::Pak>(mia::Medium::create("Famicom Disk System"));
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;

  bios = std::dynamic_pointer_cast<mia::Pak>(mia::Medium::create("Famicom"));
  result = bios->load(firmware[0].location);
  if(result != successful) {
    result.firmwareSystemName = "Famicom";
    result.firmwareType = firmware[0].type;
    result.firmwareRegion = firmware[0].region;
    result.result = noFirmware;
    return result;
  }

  system = mia::System::create("Famicom");
  result = system->load();
  if(result != successful) return result;

  if(!ares::Famicom::load(root, "[Nintendo] Famicom (NTSC-J)")) return otherError;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->scan<ares::Node::Port>("Disk Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto node = root->scan<ares::Node::Setting::String>("State")) {
    node->setValue("Disk 1: Side A");
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    port->allocate("Gamepad");
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 2")) {
    port->allocate("Gamepad");
    port->connect();
  }

  diskChangeTimer = Timer{};

  return successful;
}

auto FamicomDiskSystem::unload() -> void {
  Emulator::unload();
  diskChangeTimer.reset();
}

auto FamicomDiskSystem::save() -> bool {
  root->save();
  system->save(system->location);
  bios->save(bios->location);
  game->save(game->location);
  return true;
}

auto FamicomDiskSystem::pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> {
  if(node->name() == "Famicom") return system->pak;
  if(node->name() == "Famicom Cartridge") return bios->pak;
  if(node->name() == "Famicom Disk System") return game->pak;
  return {};
}

auto FamicomDiskSystem::notify(const string& message) -> void {
  if(auto node = root->scan<ares::Node::Setting::String>("State")) {
    node->setValue(message);
  }
}
