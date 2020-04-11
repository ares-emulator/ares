#include <ws/ws.hpp>

namespace ares::WonderSwan {

Cartridge& cartridge = cartridgeSlot.cartridge;
#include "slot.cpp"
#include "memory.cpp"
#include "rtc.cpp"
#include "io.cpp"
#include "serialization.cpp"

auto Cartridge::allocate(Node::Port parent) -> Node::Peripheral {
  string name = interface->name();
  if(Model::SwanCrystal()) name = "WonderSwan Color";
  return node = parent->append<Node::Peripheral>(name);
}

auto Cartridge::connect() -> void {
  node->setManifest([&] { return information.manifest; });

  information = {};

  if(auto fp = platform->open(node, "manifest.bml", File::Read, File::Required)) {
    information.manifest = fp->reads();
  }

  auto document = BML::unserialize(information.manifest);
  information.name = document["game/label"].string();

  if(auto memory = document["game/board/memory(type=ROM,content=Program)"]) {
    rom.size = memory["size"].natural();
    rom.mask = bit::round(rom.size) - 1;
    rom.data = new uint8[rom.mask + 1];
    memory::fill<uint8>(rom.data, rom.mask + 1, 0xff);
    if(auto fp = platform->open(node, "program.rom", File::Read, File::Required)) {
      fp->read(rom.data, rom.size);
    }
  }

  if(auto memory = document["game/board/memory(type=RAM,content=Save)"]) {
    ram.size = memory["size"].natural();
    ram.mask = bit::round(ram.size) - 1;
    ram.data = new uint8[ram.mask + 1];
    memory::fill<uint8>(ram.data, ram.mask + 1, 0xff);
    if(!memory["volatile"]) {
      if(auto fp = platform->open(node, "save.ram", File::Read)) {
        fp->read(ram.data, ram.size);
      }
    }
  }

  if(auto memory = document["game/board/memory(type=EEPROM,content=Save)"]) {
    eeprom.allocate(memory["size"].natural(), 16);
    if(auto fp = platform->open(node, "save.eeprom", File::Read)) {
      fp->read(eeprom.data, eeprom.size);
    }
  }

  if(auto memory = document["game/board/memory(type=RTC,content=Time)"]) {
    rtc.size = memory["size"].natural();
    rtc.mask = bit::round(rtc.size) - 1;
    rtc.data = new uint8[rtc.mask + 1];
    memory::fill<uint8>(rtc.data, rtc.mask + 1, 0x00);
    if(!memory["volatile"]) {
      if(auto fp = platform->open(node, "time.rtc", File::Read)) {
        fp->read(rtc.data, rtc.size);
      }
    }
  }

  if(document["game/orientation"].string() == "horizontal") information.orientation = "Horizontal";
  if(document["game/orientation"].string() == "vertical"  ) information.orientation = "Vertical";

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

  auto document = BML::unserialize(information.manifest);

  if(auto memory = document["game/board/memory(type=RAM,content=Save)"]) {
    if(!memory["volatile"]) {
      if(auto fp = platform->open(node, "save.ram", File::Write)) {
        fp->write(ram.data, ram.size);
      }
    }
  }

  if(auto memory = document["game/board/memory(type=EEPROM,content=Save)"]) {
    if(auto fp = platform->open(node, "save.eeprom", File::Write)) {
      fp->write(eeprom.data, eeprom.size);
    }
  }

  if(auto memory = document["game/board/memory(type=RTC,content=Time)"]) {
    if(!memory["volatile"]) {
      if(auto fp = platform->open(node, "time.rtc", File::Write)) {
        fp->write(rtc.data, rtc.size);
      }
    }
  }
}

auto Cartridge::main() -> void {
  if(rtc.data) {
    rtcTickSecond();
    rtcCheckAlarm();
  }
  step(3'072'000);
}

auto Cartridge::step(uint clocks) -> void {
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
