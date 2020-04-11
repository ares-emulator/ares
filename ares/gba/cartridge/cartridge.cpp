#include <gba/gba.hpp>

namespace ares::GameBoyAdvance {

Cartridge& cartridge = cartridgeSlot.cartridge;
#include "slot.cpp"
#include "mrom.cpp"
#include "sram.cpp"
#include "eeprom.cpp"
#include "flash.cpp"
#include "serialization.cpp"

Cartridge::Cartridge() {
  mrom.data = new uint8[mrom.size = 32 * 1024 * 1024];
  sram.data = new uint8[sram.size = 32 * 1024];
  eeprom.data = new uint8[eeprom.size = 8 * 1024];
  flash.data = new uint8[flash.size = 128 * 1024];
}

Cartridge::~Cartridge() {
  delete[] mrom.data;
  delete[] sram.data;
  delete[] eeprom.data;
  delete[] flash.data;
}

auto Cartridge::allocate(Node::Port parent) -> Node::Peripheral {
  return node = parent->append<Node::Peripheral>(interface->name());
}

auto Cartridge::connect() -> void {
  node->setManifest([&] { return information.manifest; });

  information = {};
  has = {};

  if(auto fp = platform->open(node, "manifest.bml", File::Read, File::Required)) {
    information.manifest = fp->reads();
  }

  auto document = BML::unserialize(information.manifest);
  information.name = document["game/label"].text();

  if(auto memory = document["game/board/memory(type=ROM,content=Program)"]) {
    mrom.size = min(32 * 1024 * 1024, memory["size"].natural());
    if(auto fp = platform->open(node, "program.rom", File::Read, File::Required)) {
      fp->read(mrom.data, mrom.size);
    }
  }

  if(auto memory = document["game/board/memory(type=RAM,content=Save)"]) {
    has.sram = true;
    sram.size = min(32 * 1024, memory["size"].natural());
    sram.mask = sram.size - 1;
    for(auto n : range(sram.size)) sram.data[n] = 0xff;

    if(!memory["volatile"]) {
      if(auto fp = platform->open(node, "save.ram", File::Read)) {
        fp->read(sram.data, sram.size);
      }
    }
  }

  if(auto memory = document["game/board/memory(type=EEPROM,content=Save)"]) {
    has.eeprom = true;
    eeprom.size = min(8 * 1024, memory["size"].natural());
    eeprom.bits = eeprom.size <= 512 ? 6 : 14;
    if(eeprom.size == 0) eeprom.size = 8192, eeprom.bits = 0;  //auto-detect size
    eeprom.mask = mrom.size > 16 * 1024 * 1024 ? 0x0fffff00 : 0x0f000000;
    eeprom.test = mrom.size > 16 * 1024 * 1024 ? 0x0dffff00 : 0x0d000000;
    for(auto n : range(eeprom.size)) eeprom.data[n] = 0xff;

    if(auto fp = platform->open(node, "save.eeprom", File::Read)) {
      fp->read(eeprom.data, eeprom.size);
    }
  }

  if(auto memory = document["game/board/memory(type=Flash,content=Save)"]) {
    has.flash = true;
    flash.size = min(128 * 1024, memory["size"].natural());
    flash.manufacturer = memory["manufacturer"].text();
    for(auto n : range(flash.size)) flash.data[n] = 0xff;

    flash.id = 0;
    if(flash.manufacturer == "Atmel"     && flash.size ==  64 * 1024) flash.id = 0x3d1f;
    if(flash.manufacturer == "Macronix"  && flash.size ==  64 * 1024) flash.id = 0x1cc2;
    if(flash.manufacturer == "Macronix"  && flash.size == 128 * 1024) flash.id = 0x09c2;
    if(flash.manufacturer == "Panasonic" && flash.size ==  64 * 1024) flash.id = 0x1b32;
    if(flash.manufacturer == "Sanyo"     && flash.size == 128 * 1024) flash.id = 0x1362;
    if(flash.manufacturer == "SST"       && flash.size ==  64 * 1024) flash.id = 0xd4bf;

    if(auto fp = platform->open(node, "save.flash", File::Read)) {
      fp->read(flash.data, flash.size);
    }
  }

  power();
}

auto Cartridge::disconnect() -> void {
  if(!node) return;
  memory::fill<uint8>(mrom.data, mrom.size);
  memory::fill<uint8>(sram.data, sram.size);
  memory::fill<uint8>(eeprom.data, eeprom.size);
  memory::fill<uint8>(flash.data, flash.size);
  has = {};
  node = {};
}

auto Cartridge::save() -> void {
  if(!node) return;
  auto document = BML::unserialize(information.manifest);

  if(auto memory = document["game/board/memory(type=RAM,content=Save)"]) {
    if(!memory["volatile"]) {
      if(auto fp = platform->open(node, "save.ram", File::Write)) {
        fp->write(sram.data, sram.size);
      }
    }
  }

  if(auto memory = document["game/board/memory(type=EEPROM,content=Save)"]) {
    if(auto fp = platform->open(node, "save.eeprom", File::Write)) {
      fp->write(eeprom.data, eeprom.size);
    }
  }

  if(auto memory = document["game/board/memory(type=Flash,content=Save)"]) {
    if(auto fp = platform->open(node, "save.flash", File::Write)) {
      fp->write(flash.data, flash.size);
    }
  }
}

auto Cartridge::power() -> void {
  eeprom.power();
  flash.power();
}

#define RAM_ANALYZE

auto Cartridge::read(uint mode, uint32 address) -> uint32 {
  if(address < 0x0e00'0000) {
    if(has.eeprom && (address & eeprom.mask) == eeprom.test) return eeprom.read();
    return mrom.read(mode, address);
  } else {
    if(has.sram) return sram.read(mode, address);
    if(has.flash) return flash.read(address);
    return cpu.pipeline.fetch.instruction;
  }
}

auto Cartridge::write(uint mode, uint32 address, uint32 word) -> void {
  if(address < 0x0e00'0000) {
    if(has.eeprom && (address & eeprom.mask) == eeprom.test) return eeprom.write(word & 1);
    return mrom.write(mode, address, word);
  } else {
    if(has.sram) return sram.write(mode, address, word);
    if(has.flash) return flash.write(address, word);
  }
}

}
