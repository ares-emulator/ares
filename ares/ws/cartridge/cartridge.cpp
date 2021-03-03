#include <ws/ws.hpp>

namespace ares::WonderSwan {

Cartridge& cartridge = cartridgeSlot.cartridge;
#include "slot.cpp"
#include "memory.cpp"
#include "rtc.cpp"
#include "io.cpp"
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
    rom.size = fp->size();
    rom.mask = bit::round(rom.size) - 1;
    rom.data = new n8[rom.mask + 1];
    memory::fill<u8>(rom.data, rom.mask + 1, 0xff);
    fp->read({rom.data, rom.size});
  }

  if(auto fp = pak->read("save.ram")) {
    ram.size = fp->size();
    ram.mask = bit::round(ram.size) - 1;
    ram.data = new n8[ram.mask + 1];
    memory::fill<u8>(ram.data, ram.mask + 1, 0xff);
    fp->read({ram.data, ram.size});
  }

  if(auto fp = pak->read("save.eeprom")) {
    eeprom.allocate(fp->size(), 16, 1, 0xff);
    fp->read({eeprom.data, eeprom.size});
  }

  if(auto fp = pak->read("time.rtc")) {
    rtc.size = fp->size();
    rtc.mask = bit::round(rtc.size) - 1;
    rtc.data = new n8[rtc.mask + 1];
    memory::fill<u8>(rtc.data, rtc.mask + 1, 0x00);
    fp->read({rtc.data, rtc.size});
  }

  power();
}

auto Cartridge::disconnect() -> void {
  if(!node) return;
  Thread::destroy();

  delete[] rom.data;
  rom.data = nullptr;
  rom.size = 0;
  rom.mask = 0;

  delete[] ram.data;
  ram.data = nullptr;
  ram.size = 0;
  ram.mask = 0;

  delete[] rtc.data;
  rtc.data = nullptr;
  rtc.size = 0;
  rtc.mask = 0;
}

auto Cartridge::save() -> void {
  if(!node) return;

  if(auto fp = pak->write("save.ram")) {
    fp->write({ram.data, ram.size});
  }

  if(auto fp = pak->write("save.eeprom")) {
    fp->write({eeprom.data, eeprom.size});
  }

  if(auto fp = pak->write("time.rtc")) {
    fp->write({rtc.data, rtc.size});
  }
}

auto Cartridge::main() -> void {
  if(rtc.data) {
    rtcTickSecond();
    rtcCheckAlarm();
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

  bus.map(this, 0x00c0, 0x00c8);
  if(rtc.data) bus.map(this, 0x00ca, 0x00cb);
  bus.map(this, 0x00cc, 0x00cd);

  r = {};
}

}
