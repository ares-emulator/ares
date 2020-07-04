#include <ps1/interface/interface.hpp>

struct PlayStation : Emulator {
  PlayStation();
  auto load() -> bool override;
  auto open(ares::Node::Object, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> override;
  auto input(ares::Node::Input) -> void override;

  uint regionID = 0;
};

PlayStation::PlayStation() {
  interface = new ares::PlayStation::PlayStationInterface;
  medium = mia::medium("PlayStation");
  manufacturer = "Sony";
  name = "PlayStation";

  firmware.append({"BIOS", "US"});      //NTSC-U
  firmware.append({"BIOS", "Japan"});   //NTSC-J
  firmware.append({"BIOS", "Europe"});  //PAL
}

auto PlayStation::load() -> bool {
  if(!file::exists(firmware[regionID].location)) {
    errorFirmwareRequired(firmware[regionID]);
    return false;
  }

  if(auto port = root->find<ares::Node::Port>("PlayStation/Disc Tray")) {
    port->allocate();
    port->connect();
  }

  return true;
}

auto PlayStation::open(ares::Node::Object node, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> {
  if(name == "bios.rom") {
    return Emulator::loadFirmware(firmware[regionID]);
  }

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

  return {};
}

auto PlayStation::input(ares::Node::Input node) -> void {
}
