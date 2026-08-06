#include <n64/n64.hpp>
#include <nall/tcptext/tcp-socket.hpp>
#include <deque>
#include <vector>

namespace ares::Nintendo64 {

#include "sc64.hpp"
#include "sc64.cpp"

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

  if(system.sc64SDImage || system.sc64USBHostPort) {
    auto device = std::make_unique<SC64>(*this);
    if(device->open(system.sc64SDImage, system.sc64SDImageReadOnly, system.sc64USBHostPort)) {
      sc64 = std::move(device);
    }
  }

  // A real SC64 owns the entire PI ROM window through its own SDRAM; attaching
  // romDevice as well would only let it shadow that window behind sc64.
  if(!sc64) pi.attach(romDevice, 0);
  if(ram) pi.attach(ramDevice, 1);
  if(flash) pi.attach(flash, 1);
  // On a real SC64, 0x13FF0000 is plain SDRAM: ISViewer output is captured by
  // the cart firmware polling that memory (SC64::pollIsViewer), not by bus
  // hardware, so ares's ISViewer device must not shadow it.
  if(isviewer.enabled() && !sc64) pi.attach(isviewer, 1);
  if(sc64) pi.attach(*sc64, 1);

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
  if(sc64) pi.detach(*sc64);
  debugger.unload(node);
  rom.reset();
  ram.reset();
  eeprom.reset();
  flash.reset();
  isviewer.ram.reset();
  if(sc64) sc64->close();
  sc64.reset();
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
  if(sc64) sc64->power(reset);
}

auto Cartridge::pollSc64Host() -> void {
  if(sc64) sc64->pollHost();
}

auto Cartridge::irqLine() const -> bool {
  return sc64 && sc64->irqLine();
}

}
