struct FamicomDiskSystem : Emulator {
  FamicomDiskSystem();
  auto load(Menu) -> void override;
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;
  auto notify(const string& message) -> void override;

  shared_pointer<mia::Pak> bios;
};

FamicomDiskSystem::FamicomDiskSystem() {
  manufacturer = "Nintendo";
  name = "Famicom Disk System";

  firmware.append({"BIOS", "Japan"});
}

auto FamicomDiskSystem::load(Menu menu) -> void {
  Group group;
  Menu diskMenu{&menu};
  diskMenu.setText("Disk Drive").setIcon(Icon::Media::Floppy);

  MenuRadioItem ejected{&diskMenu};
  ejected.setText("No Disk").onActivate([&] { emulator->notify("Ejected"); });
  group.append(ejected);
  if(game->pak->count() < 2) return (void)ejected.setChecked();

  MenuRadioItem disk1sideA{&diskMenu};
  disk1sideA.setText("Disk 1: Side A").onActivate([&] { emulator->notify("Disk 1: Side A"); });
  group.append(disk1sideA);
  if(game->pak->count() < 3) return (void)disk1sideA.setChecked();

  MenuRadioItem disk1sideB{&diskMenu};
  disk1sideB.setText("Disk 1: Side B").onActivate([&] { emulator->notify("Disk 1: Side B"); });
  group.append(disk1sideB);
  if(game->pak->count() < 4) return (void)disk1sideA.setChecked();

  MenuRadioItem disk2sideA{&diskMenu};
  disk2sideA.setText("Disk 2: Side A").onActivate([&] { emulator->notify("Disk 2: Side A"); });
  group.append(disk2sideA);
  if(game->pak->count() < 5) return (void)disk1sideA.setChecked();

  MenuRadioItem disk2sideB{&diskMenu};
  disk2sideB.setText("Disk 2: Side B").onActivate([&] { emulator->notify("Disk 2: Side B"); });
  group.append(disk2sideB);
  return (void)disk1sideA.setChecked();
}

auto FamicomDiskSystem::load() -> bool {
  game = mia::Medium::create("Famicom Disk");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  bios = mia::Medium::create("Famicom");
  if(!bios->load(firmware[0].location)) return errorFirmware(firmware[0]), false;

  system = mia::System::create("Famicom");
  if(!system->load()) return false;

  if(!ares::Famicom::load(root, "[Nintendo] Famicom (NTSC-J)")) return false;

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

  return true;
}

auto FamicomDiskSystem::save() -> bool {
  root->save();
  system->save(system->location);
  bios->save(bios->location);
  game->save(game->location);
  return true;
}

auto FamicomDiskSystem::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Famicom") return system->pak;
  if(node->name() == "Famicom Cartridge") return bios->pak;
  if(node->name() == "Famicom Disk") return game->pak;
  return {};
}

auto FamicomDiskSystem::input(ares::Node::Input::Input node) -> void {
  auto name = node->name();
  maybe<InputMapping&> mapping;
  if(name == "Up"        ) mapping = virtualPads[0].up;
  if(name == "Down"      ) mapping = virtualPads[0].down;
  if(name == "Left"      ) mapping = virtualPads[0].left;
  if(name == "Right"     ) mapping = virtualPads[0].right;
  if(name == "B"         ) mapping = virtualPads[0].a;
  if(name == "A"         ) mapping = virtualPads[0].b;
  if(name == "Select"    ) mapping = virtualPads[0].select;
  if(name == "Start"     ) mapping = virtualPads[0].start;
  if(name == "Microphone") mapping = virtualPads[0].x;

  if(mapping) {
    auto value = mapping->value();
    if(auto button = node->cast<ares::Node::Input::Button>()) {
       button->setValue(value);
    }
  }
}

auto FamicomDiskSystem::notify(const string& message) -> void {
  if(auto node = root->scan<ares::Node::Setting::String>("State")) {
    node->setValue(message);
  }
}
