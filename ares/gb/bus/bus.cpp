#include <gb/gb.hpp>

namespace ares::GameBoy {

Bus bus;

auto Bus::read(uint cycle, uint16 address, uint8 data) -> uint8 {
  data &= cpu.readIO(cycle, address, data);
  data &= apu.readIO(cycle, address, data);
  data &= ppu.readIO(cycle, address, data);
  data &= cartridge.readIO(cycle, address, data);
  return data;
}

auto Bus::write(uint cycle, uint16 address, uint8 data) -> void {
  cpu.writeIO(cycle, address, data);
  apu.writeIO(cycle, address, data);
  ppu.writeIO(cycle, address, data);
  cartridge.writeIO(cycle, address, data);
}

auto Bus::read(uint16 address, uint8 data) -> uint8 {
//data &= read(0, address, data);
//data &= read(1, address, data);
  data &= read(2, address, data);
//data &= read(3, address, data);
  data &= read(4, address, data);
  return data;
}

auto Bus::write(uint16 address, uint8 data) -> void {
//write(0, address, data);
//write(1, address, data);
  write(2, address, data);
//write(3, address, data);
  write(4, address, data);
}

}
