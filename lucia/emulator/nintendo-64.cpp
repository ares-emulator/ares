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

  firmware.append({"PIF", "NTSC"});
  firmware.append({"PIF", "PAL"});
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
  if(name == "X-axis" );
  if(name == "Y-axis" );
  if(name == "Up"     ) mapping = virtualPad.up;
  if(name == "Down"   ) mapping = virtualPad.down;
  if(name == "Left"   ) mapping = virtualPad.left;
  if(name == "Right"  ) mapping = virtualPad.right;
  if(name == "B"      ) mapping = virtualPad.a;
  if(name == "A"      ) mapping = virtualPad.b;
  if(name == "C-Up"   );
  if(name == "C-Down" );
  if(name == "C-Left" );
  if(name == "C-Right");
  if(name == "L"      ) mapping = virtualPad.l;
  if(name == "R"      ) mapping = virtualPad.r;
  if(name == "Z"      ) mapping = virtualPad.select;
  if(name == "Start"  ) mapping = virtualPad.start;

  if(mapping) {
    auto value = mapping->value();
    if(auto button = node->cast<ares::Node::Button>()) {
      button->setValue(value);
    }
  }
}
