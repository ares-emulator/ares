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
  name = "Mega Drive";
  extensions = {"md", "smd", "gen"};
}

auto MegaDrive::load() -> bool {
  if(auto region = root->find<ares::Node::String>("Region")) {
    region->setValue("NTSC-U → NTSC-J → PAL");
  }

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    auto peripheral = port->allocate();
    port->connect(peripheral);
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    auto peripheral = port->allocate();
    peripheral->setName("Fighting Pad");
    port->connect(peripheral);
  }

  return true;
}

auto MegaDrive::open(ares::Node::Object node, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> {
  if(name == "manifest.bml") return Emulator::manifest();

  auto document = BML::unserialize(game.manifest);
  auto programROMSize = document["game/board/memory(content=Program,type=ROM)/size"].natural();
  auto saveRAMVolatile = (bool)document["game/board/memory(Content=Save,type=RAM)/volatile"];

  if(name == "program.rom") {
    return vfs::memory::file::open(game.image.data(), programROMSize);
  }

  if(name == "save.ram" && !saveRAMVolatile) {
    auto location = locate(game.location, ".sav", settings.paths.saves);
    if(auto result = vfs::fs::file::open(location, mode)) return result;
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
  name = "Mega CD";
  extensions = {"bin", "img"};

  firmware.append({"BIOS", "US"});
  firmware.append({"BIOS", "Japan"});
  firmware.append({"BIOS", "Europe"});
}

auto MegaCD::load() -> bool {
  //todo: implement this into mia::MegaCD
  regionID = 0;
  if(file::size(game.location) >= 0x210) {
    auto fp = file::open(game.location, file::mode::read);
    fp.seek(0x200);
    uint8_t region = fp.read();
    if(region == 'U') regionID = 0;
    if(region == 'J') regionID = 1;
    if(region == 'E') regionID = 2;
    if(region == 'W') regionID = 0;
  }

  if(!file::exists(firmware[regionID].location)) {
    errorFirmwareRequired(firmware[regionID]);
    return false;
  }

  if(auto region = root->find<ares::Node::String>("Region")) {
    region->setValue("NTSC-U → NTSC-J → PAL");
  }

  if(auto port = root->find<ares::Node::Port>("Expansion Port")) {
    auto peripheral = port->allocate();
    port->connect(peripheral);
  }

  if(auto port = root->scan<ares::Node::Port>("Disc Tray")) {
    auto peripheral = port->allocate();
    port->connect(peripheral);
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    auto peripheral = port->allocate();
    peripheral->setName("Fighting Pad");
    port->connect(peripheral);
  }

  return true;
}

auto MegaCD::open(ares::Node::Object node, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> {
  if(node->name() == "Mega Drive") {
    if(name == "manifest.bml") {
      for(auto& media : mia::media) {
        if(media->name() != "Mega Drive") continue;
        if(auto cartridge = media.cast<mia::Cartridge>()) {
          if(auto image = loadFirmware(firmware[regionID])) {
            vector<uint8_t> bios;
            bios.resize(image->size());
            image->read(bios.data(), bios.size());
            auto manifest = cartridge->manifest(bios, firmware[regionID].location);
            return vfs::memory::file::open(manifest.data<uint8_t>(), manifest.size());
          }
        }
      }
    }

    if(name == "program.rom") {
      return loadFirmware(firmware[regionID]);
    }

    if(name == "backup.ram") {
      auto location = locate(game.location, ".sav", settings.paths.saves);
      if(auto result = vfs::fs::file::open(location, mode)) return result;
    }
  }

  if(node->name() == "Mega CD") {
    if(name == "manifest.bml") {
      string manifest;
      manifest.append("game\n");
      manifest.append("  name:  ", Location::prefix(game.location), "\n");
      manifest.append("  label: ", Location::prefix(game.location), "\n");
      return vfs::memory::file::open(manifest.data<uint8_t>(), manifest.size());
    }

    if(name == "cd.rom") {
      if(game.location.iendsWith(".zip")) {
        MessageDialog().setText(
          "Sorry, compressed CD-ROM images are not currently supported.\n"
          "Please extract the image prior to loading it."
        ).setAlignment(presentation).error();
        return {};
      }

      string binLocation = game.location;
      string cueLocation = {Location::notsuffix(game.location), ".cue"};
      string subLocation = {Location::notsuffix(game.location), ".sub"};
      if(auto result = vfs::fs::cdrom::open(binLocation, cueLocation, subLocation)) return result;
      MessageDialog().setText(
        "Failed to load CD-ROM image.\n"
        "Please ensure image is in single-file BIN+CUE format and is uncompressed."
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
