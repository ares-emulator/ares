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
  node->setManifest([&] { return information.manifest; });

  information = {};

  if(auto fp = platform->open(node, "manifest.bml", File::Read, File::Required)) {
    information.manifest = fp->reads();
  }

  auto document = BML::unserialize(information.manifest);
  information.name = document["game/label"].string();
  information.region = document["game/region"].string();
  information.cic = document["game/board/cic"].string();

  if(auto memory = document["game/board/memory(type=ROM,content=Program)"]) {
    rom.allocate(memory["size"].natural());
    if(auto fp = platform->open(node, "program.rom", File::Read, File::Required)) {
      rom.load(fp);
    }
  } else {
    rom.allocate(16);
  }

  if(auto memory = document["game/board/memory(type=RAM,content=Save)"]) {
    ram.allocate(memory["size"].natural());
    if(auto fp = platform->open(node, "save.ram", File::Read)) {
      ram.load(fp);
    }
  }

  if(auto memory = document["game/board/memory(type=EEPROM,content=Save)"]) {
    eeprom.allocate(memory["size"].natural());
    if(auto fp = platform->open(node, "save.eeprom", File::Read)) {
      eeprom.load(fp);
    }
  }

  if(auto memory = document["game/board/memory(type=Flash,content=Save)"]) {
    flash.allocate(memory["size"].natural());
    if(auto fp = platform->open(node, "save.flash", File::Read)) {
      flash.load(fp);
    }
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
  node.reset();
}

auto Cartridge::save() -> void {
  if(!node) return;
  auto document = BML::unserialize(information.manifest);

  if(auto memory = document["game/board/memory(type=RAM,content=Save)"]) {
    if(auto fp = platform->open(node, "save.ram", File::Write)) {
      ram.save(fp);
    }
  }

  if(auto memory = document["game/board/memory(type=EEPROM,content=Save)"]) {
    if(auto fp = platform->open(node, "save.eeprom", File::Write)) {
      eeprom.save(fp);
    }
  }

  if(auto memory = document["game/board/memory(type=Flash,content=Save)"]) {
    if(auto fp = platform->open(node, "save.flash", File::Write)) {
      flash.save(fp);
    }
  }
}

auto Cartridge::power(bool reset) -> void {
  flash.mode = Flash::Mode::Idle;
  flash.status = 0;
  flash.source = 0;
  flash.offset = 0;
}

}
