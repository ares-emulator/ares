#include <md/interface/interface.hpp>

struct MegaDrive : Emulator {
  MegaDrive();
  auto load() -> bool override;
  auto open(ares::Node::Object, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> override;
  auto input(ares::Node::Input) -> void override;
};

struct MegaCD : Emulator {
  MegaCD();
  auto load() -> bool override;
  auto open(ares::Node::Object, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> override;
  auto input(ares::Node::Input) -> void override;

  uint regionID = 0;
};

MegaDrive::MegaDrive() {
  interface = new ares::MegaDrive::MegaDriveInterface;
  medium = mia::medium("Mega Drive");
  manufacturer = "Sega";
  name = "Mega Drive";
}

auto MegaDrive::load() -> bool {
  if(auto region = root->find<ares::Node::String>("Region")) {
    region->setValue("NTSC-U → NTSC-J → PAL");
  }

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    port->allocate("Fighting Pad");
    port->connect();
  }

  return true;
}

auto MegaDrive::open(ares::Node::Object node, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> {
  if(name == "manifest.bml") return Emulator::manifest();

  auto document = BML::unserialize(game.manifest);
  auto programROMSize = document["game/board/memory(content=Program,type=ROM)/size"].natural();
  auto saveRAMVolatile = (bool)document["game/board/memory(Content=Save,type=RAM)/volatile"];

  if(name == "program.rom") {
    return vfs::memory::open(game.image.data(), programROMSize);
  }

  if(name == "save.ram" && !saveRAMVolatile) {
    auto location = locate(game.location, ".sav", settings.paths.saves);
    if(auto result = vfs::disk::open(location, mode)) return result;
  }

  return {};
}

auto MegaDrive::input(ares::Node::Input node) -> void {
  auto name = node->name();
  maybe<InputMapping&> mapping;
  if(name == "Up"   ) mapping = virtualPad.up;
  if(name == "Down" ) mapping = virtualPad.down;
  if(name == "Left" ) mapping = virtualPad.left;
  if(name == "Right") mapping = virtualPad.right;
  if(name == "A"    ) mapping = virtualPad.x;
  if(name == "B"    ) mapping = virtualPad.a;
  if(name == "C"    ) mapping = virtualPad.b;
  if(name == "X"    ) mapping = virtualPad.y;
  if(name == "Y"    ) mapping = virtualPad.l;
  if(name == "Z"    ) mapping = virtualPad.r;
  if(name == "Mode" ) mapping = virtualPad.select;
  if(name == "Start") mapping = virtualPad.start;

  if(mapping) {
    auto value = mapping->value();
    if(auto button = node->cast<ares::Node::Button>()) {
      button->setValue(value);
    }
  }
}

MegaCD::MegaCD() {
  interface = new ares::MegaDrive::MegaDriveInterface;
  medium = mia::medium("Mega CD");
  manufacturer = "Sega";
  name = "Mega CD";

  firmware.append({"BIOS", "US"});      //NTSC-U
  firmware.append({"BIOS", "Japan"});   //NTSC-J
  firmware.append({"BIOS", "Europe"});  //PAL
}

auto MegaCD::load() -> bool {
  regionID = 0;  //default to NTSC-U region
  if(auto manifest = medium->manifest(game.location)) {
    auto document = BML::unserialize(manifest);
    auto regions = document["game/region"].string().split(",");
    //if statements below are ordered by lowest to highest priority
    if(regions.find("PAL"   )) regionID = 2;
    if(regions.find("NTSC-J")) regionID = 1;
    if(regions.find("NTSC-U")) regionID = 0;
  }

  if(!file::exists(firmware[regionID].location)) {
    errorFirmwareRequired(firmware[regionID]);
    return false;
  }

  if(auto region = root->find<ares::Node::String>("Region")) {
    region->setValue("NTSC-U → NTSC-J → PAL");
  }

  if(auto port = root->find<ares::Node::Port>("Expansion Port")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->scan<ares::Node::Port>("Disc Tray")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    port->allocate("Fighting Pad");
    port->connect();
  }

  return true;
}

auto MegaCD::open(ares::Node::Object node, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> {
  if(node->name() == "Mega Drive") {
    if(name == "manifest.bml") {
      return Emulator::manifest("Mega Drive", firmware[regionID].location);
    }

    if(name == "program.rom") {
      return Emulator::loadFirmware(firmware[regionID]);
    }

    if(name == "backup.ram") {
      auto location = locate(game.location, ".sav", settings.paths.saves);
      if(auto result = vfs::disk::open(location, mode)) return result;
    }
  }

  if(node->name() == "Mega CD") {
    if(name == "manifest.bml") {
      if(auto manifest = medium->manifest(game.location)) {
        return vfs::memory::open(manifest.data<uint8_t>(), manifest.size());
      }
      return Emulator::manifest(game.location);
    }

    if(name == "cd.rom") {
      if(game.location.iendsWith(".zip")) {
        MessageDialog().setText(
          "Sorry, compressed CD-ROM images are not currently supported.\n"
          "Please extract the image prior to loading it."
        ).setAlignment(presentation).error();
        return {};
      }

      if(auto result = vfs::cdrom::open(game.location)) return result;

      MessageDialog().setText(
        "Failed to load CD-ROM image."
      ).setAlignment(presentation).error();
    }
  }

  return {};
}

auto MegaCD::input(ares::Node::Input node) -> void {
  auto name = node->name();
  maybe<InputMapping&> mapping;
  if(name == "Up"   ) mapping = virtualPad.up;
  if(name == "Down" ) mapping = virtualPad.down;
  if(name == "Left" ) mapping = virtualPad.left;
  if(name == "Right") mapping = virtualPad.right;
  if(name == "A"    ) mapping = virtualPad.x;
  if(name == "B"    ) mapping = virtualPad.a;
  if(name == "C"    ) mapping = virtualPad.b;
  if(name == "X"    ) mapping = virtualPad.y;
  if(name == "Y"    ) mapping = virtualPad.l;
  if(name == "Z"    ) mapping = virtualPad.r;
  if(name == "Mode" ) mapping = virtualPad.select;
  if(name == "Start") mapping = virtualPad.start;

  if(mapping) {
    auto value = mapping->value();
    if(auto button = node->cast<ares::Node::Button>()) {
      button->setValue(value);
    }
  }
}
