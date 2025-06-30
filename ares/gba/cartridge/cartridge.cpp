#include <gba/gba.hpp>

namespace ares::GameBoyAdvance {

Cartridge& cartridge = cartridgeSlot.cartridge;
#include "slot.cpp"
#include "mrom.cpp"
#include "sram.cpp"
#include "eeprom.cpp"
#include "flash.cpp"
#include "gpio.cpp"
#include "rtc.cpp"
#include "serialization.cpp"

Cartridge::Cartridge() {
  sram.data = new n8[sram.size = 32 * 1024];
  eeprom.data = new n8[eeprom.size = 8 * 1024];
  flash.data = new n8[flash.size = 128 * 1024];
}

Cartridge::~Cartridge() {
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
    mrom.data.allocate(mrom.size >> 1);
    mrom.data.load(fp);
    mrom.mirror = pak->attribute("mirror").boolean();
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
    if(!fp->end()) fp->read({eeprom.data, eeprom.size});  //only load save file if already present
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

  if(auto fp = pak->read("time.rtc")) {
    has.rtc = true;
    for(auto n : range(rtc.size)) rtc.data[n] = 0x00;
    if(!fp->end()) fp->read({rtc.data, rtc.size});  //only load save file if already present
    rtc.load();
  }

  power();
}

auto Cartridge::disconnect() -> void {
  if(!node) return;
  mrom.data.reset();
  memory::fill<u8>(sram.data, sram.size);
  memory::fill<u8>(eeprom.data, eeprom.size);
  memory::fill<u8>(flash.data, flash.size);
  memory::fill<u8>(rtc.data, rtc.size);
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
    if(eeprom.bits != 0 && fp->size() != eeprom.size) fp->resize(eeprom.size);
    fp->write({eeprom.data, eeprom.size});
  }

  if(auto fp = pak->write("save.flash")) {
    fp->write({flash.data, flash.size});
  }

  if(auto fp = pak->write("time.rtc")) {
    rtc.save();
    fp->write({rtc.data, rtc.size});
  }
}

auto Cartridge::power() -> void {
  eeprom.power();
  flash.power();
  if(has.rtc) rtc.power();
}

}
