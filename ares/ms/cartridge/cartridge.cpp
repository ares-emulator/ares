#include <ms/ms.hpp>

namespace ares::MasterSystem {

Cartridge& cartridge = cartridgeSlot.cartridge;
#include "slot.cpp"
#include "mapper.cpp"
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

  if(auto fp = pak->read("save.ram")) {
    ram.allocate(fp->size());
    ram.load(fp);
  }

  power();
}

auto Cartridge::disconnect() -> void {
  if(!node) return;
  rom.reset();
  ram.reset();
  pak.reset();
  node.reset();
}

auto Cartridge::save() -> void {
  if(auto fp = pak->write("save.ram")) {
    ram.save(fp);
  }
}

auto Cartridge::power() -> void {
  mapper = {};
}

}
