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

  return true;
}

auto PlayStation::open(ares::Node::Object node, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> {
  if(name == "bios.rom") {
    return Emulator::loadFirmware(firmware[regionID]);
  }

  return {};
}

auto PlayStation::input(ares::Node::Input node) -> void {
}
