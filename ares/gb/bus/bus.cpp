#include <gb/gb.hpp>

namespace ares::GameBoy {

Bus bus;
#include "megaduck.cpp"

auto Bus::read(u32 cycle, n16 address, n8 data) -> n8 {
  if(auto result = platform->cheat(address)) return *result;

  n16 target = address;
  if(Model::MegaDuck()) {
    auto translated = megaDuckAddress(address);
    if(!translated) return data;
    target = *translated;
    //CPU::read calls this once per cycle and ands the results together, so
    //data already went out in Mega Duck layout - translate it back before
    //the components see it, or LCDC's bit order (a five-cycle) compounds
    //across all five passes back to a no-op
    data = megaDuckWriteData(address, data);
  }

  data &= cpu.readIO(cycle, target, data);
  data &= apu.readIO(cycle, target, data);
  data &= ppu.readIO(cycle, target, data);
  data &= cartridge.read(cycle, target, data);

  if(Model::MegaDuck()) data = megaDuckReadData(address, data);
  return data;
}

auto Bus::write(u32 cycle, n16 address, n8 data) -> void {
  n16 target = address;
  if(Model::MegaDuck()) {
    auto translated = megaDuckAddress(address);
    if(!translated) return;
    target = *translated;
    data = megaDuckWriteData(address, data);
  }

  cpu.writeIO(cycle, target, data);
  apu.writeIO(cycle, target, data);
  ppu.writeIO(cycle, target, data);
  cartridge.write(cycle, target, data);
}

auto Bus::read(n16 address, n8 data) -> n8 {
//data &= read(0, address, data);
//data &= read(1, address, data);
  data &= read(2, address, data);
//data &= read(3, address, data);
  data &= read(4, address, data);
  return data;
}

auto Bus::write(n16 address, n8 data) -> void {
//write(0, address, data);
//write(1, address, data);
  write(2, address, data);
//write(3, address, data);
  write(4, address, data);
}

}
