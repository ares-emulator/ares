#include <n64/interface/interface.hpp>

struct Nintendo64 : Emulator {
  Nintendo64();
  auto load() -> bool override;
  auto open(ares::Node::Object, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file>;
  auto input(ares::Node::Input) -> void override;
};

Nintendo64::Nintendo64() {
  interface = new ares::Nintendo64::Nintendo64Interface;
  medium = mia::medium("Nintendo 64");
  name = "Nintendo 64";
}

auto Nintendo64::load() -> bool {
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

auto Nintendo64::open(ares::Node::Object node, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> {
  if(name == "manifest.bml") return Emulator::manifest();

  if(name == "pif.rom") {
    return vfs::memory::open(Resource::Nintendo64::PIF::ROM, sizeof Resource::Nintendo64::PIF::ROM);
  }

  if(name == "pif.ntsc.rom") {
    return vfs::memory::open(Resource::Nintendo64::PIF::NTSC, sizeof Resource::Nintendo64::PIF::NTSC);
  }

  if(name == "pif.pal.rom") {
    return vfs::memory::open(Resource::Nintendo64::PIF::PAL, sizeof Resource::Nintendo64::PIF::PAL);
  }

  auto document = BML::unserialize(game.manifest);
  auto programROMSize = document["game/board/memory(content=Program,type=ROM)/size"].natural();

  if(name == "program.rom") {
    return vfs::memory::open(game.image.data(), programROMSize);
  }

  return {};
}

auto Nintendo64::input(ares::Node::Input node) -> void {
  auto name = node->name();
  maybe<InputMapping&> mapping;
  if(name == "X-axis" ) mapping = virtualPad.xAxis;
  if(name == "Y-axis" ) mapping = virtualPad.yAxis;
  if(name == "Up"     ) mapping = virtualPad.up;
  if(name == "Down"   ) mapping = virtualPad.down;
  if(name == "Left"   ) mapping = virtualPad.left;
  if(name == "Right"  ) mapping = virtualPad.right;
  if(name == "B"      ) mapping = virtualPad.a;
  if(name == "A"      ) mapping = virtualPad.b;
  if(name == "C-Up"   ) mapping = virtualPad.c;
  if(name == "C-Down" ) mapping = virtualPad.x;
  if(name == "C-Left" ) mapping = virtualPad.y;
  if(name == "C-Right") mapping = virtualPad.z;
  if(name == "L"      ) mapping = virtualPad.l;
  if(name == "R"      ) mapping = virtualPad.r;
  if(name == "Z"      ) mapping = virtualPad.select;
  if(name == "Start"  ) mapping = virtualPad.start;

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
