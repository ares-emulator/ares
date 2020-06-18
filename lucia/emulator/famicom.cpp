#include <fc/interface/interface.hpp>

struct Famicom : Emulator {
  Famicom();
  auto load() -> bool override;
  auto open(ares::Node::Object, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> override;
  auto input(ares::Node::Input) -> void override;
};

struct FamicomDiskSystem : Emulator {
  FamicomDiskSystem();
  auto load() -> bool override;
  auto open(ares::Node::Object, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> override;
  auto input(ares::Node::Input) -> void override;
  auto notify(const string& message) -> void override;

  vector<uint8_t> diskSide[4];
};

Famicom::Famicom() {
  interface = new ares::Famicom::FamicomInterface;
  medium = mia::medium("Famicom");
  manufacturer = "Nintendo";
  name = "Famicom";
}

auto Famicom::load() -> bool {
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

auto Famicom::input(ares::Node::Input node) -> void {
  auto name = node->name();
  maybe<InputMapping&> mapping;
  if(name == "Up"        ) mapping = virtualPad.up;
  if(name == "Down"      ) mapping = virtualPad.down;
  if(name == "Left"      ) mapping = virtualPad.left;
  if(name == "Right"     ) mapping = virtualPad.right;
  if(name == "B"         ) mapping = virtualPad.a;
  if(name == "A"         ) mapping = virtualPad.b;
  if(name == "Select"    ) mapping = virtualPad.select;
  if(name == "Start"     ) mapping = virtualPad.start;
  if(name == "Microphone") mapping = virtualPad.x;

  if(mapping) {
    auto value = mapping->value();
    if(auto button = node->cast<ares::Node::Button>()) {
       button->setValue(value);
    }
  }
}

FamicomDiskSystem::FamicomDiskSystem() {
  interface = new ares::Famicom::FamicomInterface;
  medium = mia::medium("Famicom Disk");
  manufacturer = "Nintendo";
  name = "Famicom Disk System";

  firmware.append({"BIOS", "Japan"});
}

auto FamicomDiskSystem::load() -> bool {
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
      array_view<uint8_t> view = game.image;
      uint index = 0;
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

  if(auto node = root->scan<ares::Node::String>("State")) {
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
            vector<uint8_t> bios;
            bios.resize(image->size());
            image->read(bios.data(), bios.size());
            auto manifest = cartridge->manifest(bios, firmware[0].location);
            return vfs::memory::open(manifest.data<uint8_t>(), manifest.size());
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
      return vfs::memory::open(game.manifest.data<uint8_t>(), game.manifest.size());
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

auto FamicomDiskSystem::input(ares::Node::Input node) -> void {
  auto name = node->name();
  maybe<InputMapping&> mapping;
  if(name == "Up"        ) mapping = virtualPad.up;
  if(name == "Down"      ) mapping = virtualPad.down;
  if(name == "Left"      ) mapping = virtualPad.left;
  if(name == "Right"     ) mapping = virtualPad.right;
  if(name == "B"         ) mapping = virtualPad.a;
  if(name == "A"         ) mapping = virtualPad.b;
  if(name == "Select"    ) mapping = virtualPad.select;
  if(name == "Start"     ) mapping = virtualPad.start;
  if(name == "Microphone") mapping = virtualPad.x;

  if(mapping) {
    auto value = mapping->value();
    if(auto button = node->cast<ares::Node::Button>()) {
       button->setValue(value);
    }
  }
}

auto FamicomDiskSystem::notify(const string& message) -> void {
  if(auto node = root->scan<ares::Node::String>("State")) {
    node->setValue(message);
  }
}
