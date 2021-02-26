namespace ares::Famicom {
  auto load(Node::System& node, string name) -> bool;
}

struct Famicom : Emulator {
  Famicom();
  auto load() -> bool override;
  auto open(ares::Node::Object, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> override;
  auto input(ares::Node::Input::Input) -> void override;
};

struct FamicomDiskSystem : Emulator {
  FamicomDiskSystem();
  auto load(Menu) -> void override;
  auto load() -> bool override;
  auto open(ares::Node::Object, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> override;
  auto input(ares::Node::Input::Input) -> void override;
  auto notify(const string& message) -> void override;
};

Famicom::Famicom() {
  medium = mia::medium("Famicom");
  manufacturer = "Nintendo";
  name = "Famicom";
}

auto Famicom::load() -> bool {
  auto region = Emulator::region();
  auto system = region == "NTSC-J" ? "Famicom" : "Nintendo";
  if(!ares::Famicom::load(root, {"[Nintendo] ", system, " (", region, ")"})) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    port->allocate("Gamepad");
    port->connect();
  }

  return true;
}

auto Famicom::open(ares::Node::Object node, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> {
  if(node->name() == "Famicom") {
    if(auto fp = pak->find(name)) return fp;
    if(auto fp = Emulator::save(name, mode, "save.ram", ".sav")) return fp;
  }
  return {};
}

auto Famicom::input(ares::Node::Input::Input node) -> void {
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

FamicomDiskSystem::FamicomDiskSystem() {
  medium = mia::medium("Famicom Disk");
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
  if(pak->count() < 2) return (void)ejected.setChecked();

  MenuRadioItem disk1sideA{&diskMenu};
  disk1sideA.setText("Disk 1: Side A").onActivate([&] { emulator->notify("Disk 1: Side A"); });
  group.append(disk1sideA);
  if(pak->count() < 3) return (void)disk1sideA.setChecked();

  MenuRadioItem disk1sideB{&diskMenu};
  disk1sideB.setText("Disk 1: Side B").onActivate([&] { emulator->notify("Disk 1: Side B"); });
  group.append(disk1sideB);
  if(pak->count() < 4) return (void)disk1sideA.setChecked();

  MenuRadioItem disk2sideA{&diskMenu};
  disk2sideA.setText("Disk 2: Side A").onActivate([&] { emulator->notify("Disk 2: Side A"); });
  group.append(disk2sideA);
  if(pak->count() < 5) return (void)disk1sideA.setChecked();

  MenuRadioItem disk2sideB{&diskMenu};
  disk2sideB.setText("Disk 2: Side B").onActivate([&] { emulator->notify("Disk 2: Side B"); });
  group.append(disk2sideB);
  return (void)disk1sideA.setChecked();
}

auto FamicomDiskSystem::load() -> bool {
  if(!ares::Famicom::load(root, "[Nintendo] Famicom (NTSC-J)")) return false;

  if(!file::exists(firmware[0].location)) {
    errorFirmwareRequired(firmware[0]);
    return false;
  }

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

auto FamicomDiskSystem::open(ares::Node::Object node, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> {
  if(node->name() == "Famicom") {
    if(name == "manifest.bml") {
      for(auto& media : mia::media) {
        if(media->name() != "Famicom") continue;
        if(auto cartridge = media.cast<mia::Cartridge>()) {
          if(auto image = loadFirmware(firmware[0])) {
            vector<u8> bios;
            bios.resize(image->size());
            image->read(bios);
            auto manifest = cartridge->manifest(bios, firmware[0].location);
            return vfs::memory::open(manifest);
          }
        }
      }
    }

    if(name == "program.rom") {
      return loadFirmware(firmware[0]);
    }
  }

  if(node->name() == "Famicom Disk") {
    if(auto fp = Emulator::save(name, mode, "disk1.sideA", ".disk1.sideA.sav")) return fp;
    if(auto fp = Emulator::save(name, mode, "disk1.sideB", ".disk1.sideB.sav")) return fp;
    if(auto fp = Emulator::save(name, mode, "disk2.sideA", ".disk2.sideA.sav")) return fp;
    if(auto fp = Emulator::save(name, mode, "disk2.sideB", ".disk2.sideB.sav")) return fp;
    if(auto fp = pak->find(name)) return fp;
  }

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
