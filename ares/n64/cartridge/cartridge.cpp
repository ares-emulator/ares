#include <n64/n64.hpp>

namespace ares::Nintendo64 {

Cartridge& cartridge = cartridgeSlot.cartridge;
#include "slot.cpp"
#include "flash.cpp"
#include "serialization.cpp"

auto Cartridge::allocate(Node::Port parent) -> Node::Peripheral {
  return node = parent->append<Node::Peripheral>(system.name());
}

auto Cartridge::connect() -> void {
  if(!node->setPak(pak = platform->pak(node))) return;

  information = {};
  if(auto fp = pak->read("manifest.bml")) {
    information.manifest = fp->reads();
  }
  auto document = BML::unserialize(information.manifest);
  information.name = document["game/label"].string();
  information.region = document["game/region"].string();
  information.cic = document["game/board/cic"].string();

  if(auto memory = document["game/board/memory(type=ROM,content=Program)"]) {
    rom.allocate(memory["size"].natural());
    rom.load(pak->read("program.rom"));
  } else {
    rom.allocate(16);
  }

  if(auto memory = document["game/board/memory(type=RAM,content=Save)"]) {
    ram.allocate(memory["size"].natural());
    ram.load(pak->read("save.ram"));
  }

  if(auto memory = document["game/board/memory(type=EEPROM,content=Save)"]) {
    eeprom.allocate(memory["size"].natural());
    eeprom.load(pak->read("save.eeprom"));
  }

  if(auto memory = document["game/board/memory(type=Flash,content=Save)"]) {
    flash.allocate(memory["size"].natural());
    flash.load(pak->read("save.flash"));
  }

  power(false);
}

auto Cartridge::disconnect() -> void {
  if(!node) return;
  save();
  rom.reset();
  ram.reset();
  eeprom.reset();
  flash.reset();
  pak.reset();
  node.reset();
}

auto Cartridge::save() -> void {
  if(!node) return;
  auto document = BML::unserialize(information.manifest);

  if(auto memory = document["game/board/memory(type=RAM,content=Save)"]) {
    ram.save(pak->write("save.ram"));
  }

  if(auto memory = document["game/board/memory(type=EEPROM,content=Save)"]) {
    eeprom.save(pak->write("save.ram"));
  }

  if(auto memory = document["game/board/memory(type=Flash,content=Save)"]) {
    flash.save(pak->write("save.eeprom"));
  }
}

auto Cartridge::power(bool reset) -> void {
  flash.mode = Flash::Mode::Idle;
  flash.status = 0;
  flash.source = 0;
  flash.offset = 0;
}

}
