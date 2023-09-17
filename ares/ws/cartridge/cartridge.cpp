#include <ws/ws.hpp>
#include <nall/bcd.hpp>

namespace ares::WonderSwan {

Cartridge& cartridge = cartridgeSlot.cartridge;
#include "slot.cpp"
#include "memory.cpp"
#include "rtc.cpp"
#include "flash.cpp"
#include "karnak.cpp"
#include "io.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto Cartridge::allocate(Node::Port parent) -> Node::Peripheral {
  string name = system.name();
  if(Model::WonderSwanColor() || Model::SwanCrystal()) name = "WonderSwan Color";
  return node = parent->append<Node::Peripheral>(string{name, " Cartridge"});
}

auto Cartridge::connect() -> void {
  if(!node->setPak(pak = platform->pak(node))) return;

  information = {};
  has = {};
  information.title = pak->attribute("title");
  if(pak->attribute("orientation") == "horizontal") information.orientation = "Horizontal";
  if(pak->attribute("orientation") == "vertical"  ) information.orientation = "Vertical";

  // WS cartridges are allocated from the top (end of ROM) down
  if(auto fp = pak->read("program.flash")) {
    rom.allocate(bit::round(fp->size()));
    fp->read({rom.data() + bit::round(fp->size()) - fp->size(), fp->size()});
    has.flash = true;
  } else if(auto fp = pak->read("program.rom")) {
    rom.allocate(bit::round(fp->size()));
    fp->read({rom.data() + bit::round(fp->size()) - fp->size(), fp->size()});
  }
  
  if(auto fp = pak->read("save.ram")) {
    ram.allocate(fp->size());
    ram.load(fp);
    has.sram = true;
  }

  if(auto fp = pak->read("save.eeprom")) {
    eeprom.allocate(fp->size(), 16, 1, 0xff);
    fp->read({eeprom.data, eeprom.size});
    has.eeprom = true;
  }

  if(auto fp = pak->read("time.rtc")) {
    rtc.ram.allocate(18);
    rtc.ram.load(fp);
    rtc.load();
    has.rtc = true;
  }

  has.karnak = pak->attribute("board") == "KARNAK";

  debugger.load(node);
  power();
}

auto Cartridge::disconnect() -> void {
  if(!node) return;
  save();
  debugger.unload(node);
  rom.reset();
  ram.reset();
  rtc.reset();
  karnak.reset();
}

auto Cartridge::save() -> void {
  if(!node) return;
  
  if(auto fp = pak->write("program.flash")) {
    rom.save(fp);
  }

  if(auto fp = pak->write("save.ram")) {
    ram.save(fp);
  }

  if(auto fp = pak->write("save.eeprom")) {
    fp->write({eeprom.data, eeprom.size});
  }

  if(auto fp = pak->write("time.rtc")) {
    rtc.save();
    rtc.ram.save(fp);
  }
}

auto Cartridge::power() -> void {
  if(has.eeprom) eeprom.power();
  if(has.rtc) rtc.power();
  if(has.flash) flash.power();
  if(has.karnak) karnak.power();

  bus.map(this, 0x00c0, 0x00ff);

  io = {};
}

}
