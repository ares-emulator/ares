#include <ws/ws.hpp>

namespace ares::WonderSwan {

Cartridge& cartridge = cartridgeSlot.cartridge;
#include "slot.cpp"
#include "memory.cpp"
#include "rtc.cpp"
#include "io.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto Cartridge::allocate(Node::Port parent) -> Node::Peripheral {
  string name = system.name();
  if(Model::SwanCrystal()) name = "WonderSwan Color";
  return node = parent->append<Node::Peripheral>(string{name, " Cartridge"});
}

auto Cartridge::connect() -> void {
  if(!node->setPak(pak = platform->pak(node))) return;

  information = {};
  information.title = pak->attribute("title");
  if(pak->attribute("orientation") == "horizontal") information.orientation = "Horizontal";
  if(pak->attribute("orientation") == "vertical"  ) information.orientation = "Vertical";

  if(auto fp = pak->read("program.rom")) {
    rom.allocate(fp->size());
    rom.load(fp);
  }

  if(auto fp = pak->read("save.ram")) {
    ram.allocate(fp->size());
    ram.load(fp);
  }

  if(auto fp = pak->read("save.eeprom")) {
    eeprom.allocate(fp->size(), 16, 1, 0xff);
    fp->read({eeprom.data, eeprom.size});
  }

  if(auto fp = pak->read("time.rtc")) {
    rtc.ram.allocate(fp->size());
    rtc.ram.load(fp);
  }

  debugger.load(node);
  power();
}

auto Cartridge::disconnect() -> void {
  if(!node) return;
  save();
  debugger.unload(node);
  Thread::destroy();
  rom.reset();
  ram.reset();
  rtc.ram.reset();
}

auto Cartridge::save() -> void {
  if(!node) return;

  if(auto fp = pak->write("save.ram")) {
    ram.save(fp);
  }

  if(auto fp = pak->write("save.eeprom")) {
    fp->write({eeprom.data, eeprom.size});
  }

  if(auto fp = pak->write("time.rtc")) {
    rtc.ram.save(fp);
  }
}

auto Cartridge::main() -> void {
  if(rtc.ram) {
    rtc.tickSecond();
    rtc.checkAlarm();
  }
  step(3'072'000);
}

auto Cartridge::step(u32 clocks) -> void {
  Thread::step(clocks);
  synchronize(cpu);
}

auto Cartridge::power() -> void {
  Thread::create(3'072'000, {&Cartridge::main, this});
  eeprom.power();
  rtc.power();

  bus.map(this, 0x00c0, 0x00c8);
  if(rtc.ram) bus.map(this, 0x00ca, 0x00cb);
  bus.map(this, 0x00cc, 0x00cd);

  io = {};
}

}
