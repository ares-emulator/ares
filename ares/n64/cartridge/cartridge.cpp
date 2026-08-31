#include <n64/n64.hpp>

namespace ares::Nintendo64 {

Cartridge& cartridge = cartridgeSlot.cartridge;
#include "slot.cpp"
#include "flash.cpp"
#include "rtc.cpp"
#include "joybus.cpp"
#include "isviewer.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto Cartridge::allocate(Node::Port parent) -> Node::Peripheral {
  return node = parent->append<Node::Peripheral>(string{system.name(), " Cartridge"});
}

auto Cartridge::connect() -> void {
  if(!node->setPak(pak = platform->pak(node))) return;

  information = {};
  information.title  = pak->attribute("title");
  information.region = pak->attribute("region");
  information.cic    = pak->attribute("cic");

  if(auto fp = pak->read("program.rom")) {
    rom.allocate(fp->size());
    rom.load(fp);
  } else {
    rom.allocate(16);
  }

  if(auto fp = pak->read("save.ram")) {
    ram.allocate(fp->size());
    ram.load(fp);
  }

  if(auto fp = pak->read("save.eeprom")) {
    eeprom.allocate(fp->size());
    eeprom.load(fp);
  }

  if(auto fp = pak->read("save.flash")) {
    flash.allocate(fp->size());
    flash.load(fp);
    flash.setModel(pak->attribute("flash/model"));
  }

  rtc.load();

  if(rom.size <= 0x03ff'0000) {
    isviewer.ram.allocate(64_KiB);
    isviewer.tracer = node->append<Node::Debugger::Tracer::Notification>("ISViewer", "Cartridge");
    isviewer.tracer->setAutoLineBreak(false);
    isviewer.tracer->setTerminal(true);
  }

  if(system.sc64Enabled) {
    sc64.open(system.sc64SDImage, system.sc64SDImageReadOnly, system.sc64USBHostPort);
  }

  // SC64's SDRAM covers the ROM window; the plain ROM device must not shadow it.
  if(!system.sc64Enabled) pi.attach(romDevice, 0);
  // SC64 SRAM and FlashRAM emulation are not implemented; always use standard PI save devices.
  if(ram) pi.attach(ramDevice, 1);
  if(flash) pi.attach(flash, 1);
  // The ISViewer window is inside SC64's SDRAM; disable ISViewer if SC64 is enabled.
  if(isviewer.enabled() && !system.sc64Enabled) pi.attach(isviewer, 1);
  if(system.sc64Enabled) pi.attach(sc64, 1);

  debugger.load(node);

  power(false);
}

auto Cartridge::disconnect() -> void {
  if(!node) return;
  save();
  pi.detach(romDevice);
  pi.detach(ramDevice);
  pi.detach(flash);
  pi.detach(isviewer);
  pi.detach(sc64);
  debugger.unload(node);
  rom.reset();
  ram.reset();
  eeprom.reset();
  flash.reset();
  isviewer.ram.reset();
  sc64.close();
  pak.reset();
  node.reset();
}

auto Cartridge::save() -> void {
  if(!node) return;

  if(auto fp = pak->write("save.ram")) {
    ram.save(fp);
  }

  if(auto fp = pak->write("save.eeprom")) {
    eeprom.save(fp);
  }

  if(auto fp = pak->write("save.flash")) {
    flash.save(fp);
  }

  rtc.save();
}

auto Cartridge::power(bool reset) -> void {
  flash.power(reset);
  isviewer.ram.fill(0);
  rtc.power(reset);
}

}
