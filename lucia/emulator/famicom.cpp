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

  vector<u8> diskSide[4];
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
  if(name == "manifest.bml") return Emulator::manifest();

  auto document = BML::unserialize(game.manifest);
  auto iNESROMSize = document["game/board/memory(content=iNES,type=ROM)/size"].natural();
  auto programROMSize = document["game/board/memory(content=Program,type=ROM)/size"].natural();
  auto characterROMSize = document["game/board/memory(content=Character,type=ROM)/size"].natural();
  auto programRAMVolatile = (bool)document["game/board/memory(content=Program,type=RAM)/volatile"];

  if(name == "program.rom") {
    return vfs::memory::open(game.image.data() + iNESROMSize, programROMSize);
  }

  if(name == "character.rom") {
    return vfs::memory::open(game.image.data() + iNESROMSize + programROMSize, characterROMSize);
  }

  if(name == "save.ram" && !programRAMVolatile) {
    auto location = locate(game.location, ".sav", settings.paths.saves);
    if(auto result = vfs::disk::open(location, mode)) return result;
  }

  if(name == "save.eeprom") {
    auto location = locate(game.location, ".sav", settings.paths.saves);
    if(auto result = vfs::disk::open(location, mode)) return result;
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
  Menu diskMenu{&menu};
  diskMenu.setText("Disk Drive").setIcon(Icon::Media::Floppy);
  MenuRadioItem ejected{&diskMenu};
  ejected.setText("No Disk").onActivate([&] { emulator->notify("Ejected"); });
  MenuRadioItem disk1sideA{&diskMenu};
  disk1sideA.setText("Disk 1: Side A").onActivate([&] { emulator->notify("Disk 1: Side A"); });
  MenuRadioItem disk1sideB{&diskMenu};
  disk1sideB.setText("Disk 1: Side B").onActivate([&] { emulator->notify("Disk 1: Side B"); });
  MenuRadioItem disk2sideA{&diskMenu};
  disk2sideA.setText("Disk 2: Side A").onActivate([&] { emulator->notify("Disk 2: Side A"); });
  MenuRadioItem disk2sideB{&diskMenu};
  disk2sideB.setText("Disk 2: Side B").onActivate([&] { emulator->notify("Disk 2: Side B"); });
  Group group{&ejected, &disk1sideA, &disk1sideB, &disk2sideA, &disk2sideB};
  disk1sideA.setChecked();
}

auto FamicomDiskSystem::load() -> bool {
  if(!ares::Famicom::load(root, "[Nintendo] Famicom (NTSC-J)")) return false;

  if(!file::exists(firmware[0].location)) {
    errorFirmwareRequired(firmware[0]);
    return false;
  }

  for(auto& media : mia::media) {
    if(media->name() != "Famicom Disk") continue;
    if(auto famicomDisk = media.cast<mia::FamicomDisk>()) {
      if(game.image.size() % 65500 == 16) {
        //iNES and fwNES headers are unnecessary
        memory::move(&game.image[0], &game.image[16], game.image.size() - 16);
        game.image.resize(game.image.size() - 16);
      }
      array_view<u8> view = game.image;
      u32 index = 0;
      while(auto output = famicomDisk->transform(view)) {
        diskSide[index++] = output;
        view += 65500;
        if(index >= 4) break;
      }
    }
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
            return vfs::memory::open(manifest.data<u8>(), manifest.size());
          }
        }
      }
    }

    if(name == "program.rom") {
      return loadFirmware(firmware[0]);
    }
  }

  if(node->name() == "Famicom Disk") {
    if(name == "manifest.bml") {
      for(auto& media : mia::media) {
        if(media->name() != "Famicom Disk") continue;
        if(auto floppyDisk = media.cast<mia::FloppyDisk>()) {
          game.manifest = floppyDisk->manifest(game.image, game.location);
        }
      }
      return vfs::memory::open(game.manifest.data<u8>(), game.manifest.size());
    }

    if(name == "disk1.sideA") {
      auto location = locate(game.location, ".1A.sav", settings.paths.saves);
      if(auto result = vfs::disk::open(location, mode)) return result;
      if(mode == vfs::file::mode::read) return vfs::memory::open(diskSide[0].data(), diskSide[0].size());
    }

    if(name == "disk1.sideB") {
      auto location = locate(game.location, ".1B.sav", settings.paths.saves);
      if(auto result = vfs::disk::open(location, mode)) return result;
      if(mode == vfs::file::mode::read) return vfs::memory::open(diskSide[1].data(), diskSide[1].size());
    }

    if(name == "disk2.sideA") {
      auto location = locate(game.location, ".2A.sav", settings.paths.saves);
      if(auto result = vfs::disk::open(location, mode)) return result;
      if(mode == vfs::file::mode::read) return vfs::memory::open(diskSide[2].data(), diskSide[2].size());
    }

    if(name == "disk2.sideB") {
      auto location = locate(game.location, ".2B.sav", settings.paths.saves);
      if(auto result = vfs::disk::open(location, mode)) return result;
      if(mode == vfs::file::mode::read) return vfs::memory::open(diskSide[3].data(), diskSide[3].size());
    }
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
