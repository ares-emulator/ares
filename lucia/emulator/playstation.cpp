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
  regionID = 0;  //default to NTSC-U region (for audio CDs)
  if(auto manifest = medium->manifest(game.location)) {
    auto document = BML::unserialize(manifest);
    auto region = document["game/region"].string();
    //if statements below are ordered by lowest to highest priority
    if(region == "PAL"   ) regionID = 2;
    if(region == "NTSC-J") regionID = 1;
    if(region == "NTSC-U") regionID = 0;
  }

  if(!file::exists(firmware[regionID].location)) {
    errorFirmwareRequired(firmware[regionID]);
    return false;
  }

  if(auto region = root->find<ares::Node::String>("Region")) {
    region->setValue("NTSC-U → NTSC-J → PAL");
  }

  if(auto fastBoot = root->find<ares::Node::Boolean>("Fast Boot")) {
    fastBoot->setValue(settings.general.fastBoot);
  }

  if(auto port = root->find<ares::Node::Port>("PlayStation/Disc Tray")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    port->allocate("Gamepad");
    port->connect();
  }

  return true;
}

auto PlayStation::open(ares::Node::Object node, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> {
  if(name == "bios.rom") {
    return Emulator::loadFirmware(firmware[regionID]);
  }

  if(name == "manifest.bml") {
    if(game.manifest = medium->manifest(game.location)) {
      return vfs::memory::open(game.manifest.data<uint8_t>(), game.manifest.size());
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

  if(name == "program.exe") {
    return vfs::memory::open(game.image.data(), game.image.size());
  }

  return {};
}

auto PlayStation::input(ares::Node::Input node) -> void {
  auto name = node->name();
  maybe<InputMapping&> mapping;
  if(name == "Up"      ) mapping = virtualPad.up;
  if(name == "Down"    ) mapping = virtualPad.down;
  if(name == "Left"    ) mapping = virtualPad.left;
  if(name == "Right"   ) mapping = virtualPad.right;
  if(name == "Cross"   ) mapping = virtualPad.a;
  if(name == "Circle"  ) mapping = virtualPad.b;
  if(name == "Square"  ) mapping = virtualPad.x;
  if(name == "Triangle") mapping = virtualPad.y;
  if(name == "L1"      ) mapping = virtualPad.l;
  if(name == "L2"      );
  if(name == "R1"      ) mapping = virtualPad.r;
  if(name == "R2"      );
  if(name == "Select"  ) mapping = virtualPad.select;
  if(name == "Start"   ) mapping = virtualPad.start;
  if(name == "LX-axis" ) mapping = virtualPad.xAxis;
  if(name == "LY-axis" ) mapping = virtualPad.yAxis;
  if(name == "RX-axis" );
  if(name == "RY-axis" );
  if(name == "L-thumb" );
  if(name == "R-thumb" );

  if(mapping) {
    auto value = mapping->value();
    if(auto axis = node->cast<ares::Node::Axis>()) {
      axis->setValue(value);
    }
    if(auto button = node->cast<ares::Node::Button>()) {
      button->setValue(value);
    }
  }
}
