#include <sg/sg.hpp>

namespace ares::SG1000 {

Cartridge& cartridge = cartridgeSlot.cartridge;
#include "slot.cpp"
#include "serialization.cpp"

auto Cartridge::allocate(Node::Port parent) -> Node::Peripheral {
  return node = parent->append<Node::Peripheral>(string{system.name(), " Cartridge"});
}

auto Cartridge::connect() -> void {
  if(!node->setPak(pak = platform->pak(node))) return;

  information = {};
  information.title  = pak->attribute("title");
  information.region = pak->attribute("region");

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
}

auto Cartridge::read(n16 address) -> maybe<n8> {
  if(!node) return nothing;

  if(address >= 0x0000 && (address <= 0x7fff || address < rom.size())) {
    return rom.read(address - 0x0000);
  }

  if(address >= 0x8000 && address <= 0xbfff) {
    return ram.read(address - 0x8000);
  }

  return nothing;
}

auto Cartridge::write(n16 address, n8 data) -> bool {
  if(!node) return false;

  if(address >= 0x0000 && (address <= 0x7fff || address < rom.size())) {
    return rom.write(address - 0x0000, data), true;
  }

  if(address >= 0x8000 && address <= 0xbfff) {
    return ram.write(address - 0x8000, data), true;
  }

  return false;
}

}
