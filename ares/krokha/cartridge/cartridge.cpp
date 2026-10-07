#include <krokha/krokha.hpp>

namespace ares::Krokha {

Cartridge& cartridge = cartridgeSlot.cartridge;
#include "slot.cpp"
#include "serialization.cpp"

auto Cartridge::allocate(Node::Port parent) -> Node::Peripheral {
  return node = parent->append<Node::Peripheral>(string{system.name(), " Cartridge"});
}

auto Cartridge::connect() -> void {
  if(!node->setPak(pak = platform->pak(node))) return;

  information = {};
  information.title = pak->attribute("title");

  if(auto fp = pak->read("program.rom")) {
    rom.allocate(fp->size());
    rom.load(fp);
  }

  if(auto fp = pak->read("character.rom")) {
    chr.allocate(fp->size());
    chr.load(fp);
  }

  power();
}

auto Cartridge::disconnect() -> void {
  if(!node) return;
  rom.reset();
  chr.reset();
  pak.reset();
  node.reset();
}

auto Cartridge::save() -> void {
}

auto Cartridge::power() -> void {
}

auto Cartridge::read(n16 address) -> n8 {
  if(address >= rom.size()) return 0xff;
  return rom.read(address);
}

auto Cartridge::character(n16 address) -> n8 {
  if(address >= chr.size()) return 0x00;
  return chr.read(address);
}

}
