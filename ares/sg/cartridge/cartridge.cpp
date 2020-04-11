#include <sg/sg.hpp>

namespace ares::SG1000 {

Cartridge& cartridge = cartridgeSlot.cartridge;
#include "slot.cpp"
#include "serialization.cpp"

auto Cartridge::allocate(Node::Port parent) -> Node::Peripheral {
  return node = parent->append<Node::Peripheral>(interface->name());
}

auto Cartridge::connect() -> void {
  node->setManifest([&] { return information.manifest; });

  information = {};

  if(auto fp = platform->open(node, "manifest.bml", File::Read, File::Required)) {
    information.manifest = fp->reads();
  }

  auto document = BML::unserialize(information.manifest);
  information.name = document["game/label"].text();
  information.region = document["game/region"].text();

  if(auto memory = document["game/board/memory(type=ROM,content=Program)"]) {
    rom.allocate(memory["size"].natural());
    if(auto fp = platform->open(node, "program.rom", File::Read, File::Required)) {
      rom.load(fp);
    }
  }

  if(auto memory = document["game/board/memory(type=RAM,content=Save)"]) {
    ram.allocate(memory["size"].natural());
    if(!(bool)memory["volatile"]) {
      if(auto fp = platform->open(node, "save.ram", File::Read)) {
        ram.load(fp);
      }
    }
  }

  power();
}

auto Cartridge::disconnect() -> void {
  if(!node) return;
  rom.reset();
  ram.reset();
  node = {};
}

auto Cartridge::save() -> void {
  auto document = BML::unserialize(information.manifest);

  if(auto memory = document["game/board/memory(type=RAM,content=Save)"]) {
    if(!(bool)memory["volatile"]) {
      if(auto fp = platform->open(node, "save.ram", File::Write)) {
        ram.save(fp);
      }
    }
  }
}

auto Cartridge::power() -> void {
}

auto Cartridge::read(uint16 address) -> maybe<uint8> {
  if(!node) return nothing;

  if(address >= 0x0000 && address <= 0x7fff) {
    return rom.read(address - 0x0000);
  }

  if(address >= 0x8000 && address <= 0xbfff) {
    return ram.read(address - 0x8000);
  }

  return nothing;
}

auto Cartridge::write(uint16 address, uint8 data) -> bool {
  if(!node) return false;

  if(address >= 0x0000 && address <= 0x7fff) {
    return rom.write(address - 0x0000, data), true;
  }

  if(address >= 0x8000 && address <= 0xbfff) {
    return ram.write(address - 0x8000, data), true;
  }

  return false;
}

}
