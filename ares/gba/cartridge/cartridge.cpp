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
  mrom.data = new n8[mrom.size = 32 * 1024 * 1024];
  sram.data = new n8[sram.size = 32 * 1024];
  eeprom.data = new n8[eeprom.size = 8 * 1024];
  flash.data = new n8[flash.size = 128 * 1024];
}

Cartridge::~Cartridge() {
  delete[] mrom.data;
  delete[] sram.data;
  delete[] eeprom.data;
  delete[] flash.data;
}

auto Cartridge::allocate(Node::Port parent) -> Node::Peripheral {
  return node = parent->append<Node::Peripheral>("Game Boy Advance Cartridge");
}

auto Cartridge::connect() -> void {
  if(!node->setPak(pak = platform->pak(node))) return;

  information = {};
  has = {};
  information.title = pak->attribute("title");

  if(auto fp = pak->read("program.rom")) {
    mrom.size = min(32_MiB, fp->size());
    fp->read({mrom.data, mrom.size});
  }

  if(auto fp = pak->read("save.ram")) {
    has.sram = true;
    sram.size = min(32_KiB, fp->size());
    sram.mask = sram.size - 1;
    for(auto n : range(sram.size)) sram.data[n] = 0xff;
    fp->read({sram.data, sram.size});
  }

  if(auto fp = pak->read("save.eeprom")) {
    has.eeprom = true;
    eeprom.size = min(8_KiB, fp->size());
    eeprom.bits = eeprom.size <= 512 ? 6 : 14;
    if(eeprom.size == 0) eeprom.size = 8192, eeprom.bits = 0;  //auto-detect size
    eeprom.mask = mrom.size > 16 * 1024 * 1024 ? 0x0fffff00 : 0x0f000000;
    eeprom.test = mrom.size > 16 * 1024 * 1024 ? 0x0dffff00 : 0x0d000000;
    for(auto n : range(eeprom.size)) eeprom.data[n] = 0xff;
    fp->read({eeprom.data, eeprom.size});
  }

  if(auto fp = pak->read("save.flash")) {
    has.flash = true;
    flash.size = min(128_KiB, fp->size());
    flash.manufacturer = fp->attribute("manufacturer");
    for(auto n : range(flash.size)) flash.data[n] = 0xff;
    flash.id = 0;
    if(flash.manufacturer == "Atmel"     && flash.size ==  64 * 1024) flash.id = 0x3d1f;
    if(flash.manufacturer == "Macronix"  && flash.size ==  64 * 1024) flash.id = 0x1cc2;
    if(flash.manufacturer == "Macronix"  && flash.size == 128 * 1024) flash.id = 0x09c2;
    if(flash.manufacturer == "Panasonic" && flash.size ==  64 * 1024) flash.id = 0x1b32;
    if(flash.manufacturer == "Sanyo"     && flash.size == 128 * 1024) flash.id = 0x1362;
    if(flash.manufacturer == "SST"       && flash.size ==  64 * 1024) flash.id = 0xd4bf;
    fp->read({flash.data, flash.size});
  }

  power();
}

auto Cartridge::disconnect() -> void {
  if(!node) return;
  memory::fill<u8>(mrom.data, mrom.size);
  memory::fill<u8>(sram.data, sram.size);
  memory::fill<u8>(eeprom.data, eeprom.size);
  memory::fill<u8>(flash.data, flash.size);
  has = {};
  pak.reset();
  node.reset();
}

auto Cartridge::save() -> void {
  if(!node) return;

  if(auto fp = pak->write("save.ram")) {
    fp->write({sram.data, sram.size});
  }

  if(auto fp = pak->write("save.eeprom")) {
    fp->write({eeprom.data, eeprom.size});
  }

  if(auto fp = pak->write("save.flash")) {
    fp->write({flash.data, flash.size});
  }
}

auto Cartridge::power() -> void {
  eeprom.power();
  flash.power();
}

#define RAM_ANALYZE

auto Cartridge::read(u32 mode, n32 address) -> n32 {
  if(address < 0x0e00'0000) {
    if(has.eeprom && (address & eeprom.mask) == eeprom.test) return eeprom.read();
    return mrom.read(mode, address);
  } else {
    if(has.sram) return sram.read(mode, address);
    if(has.flash) return flash.read(address);
    return cpu.pipeline.fetch.instruction;
  }
}

auto Cartridge::write(u32 mode, n32 address, n32 word) -> void {
  if(address < 0x0e00'0000) {
    if(has.eeprom && (address & eeprom.mask) == eeprom.test) return eeprom.write(word & 1);
    return mrom.write(mode, address, word);
  } else {
    if(has.sram) return sram.write(mode, address, word);
    if(has.flash) return flash.write(address, word);
  }
}

}
