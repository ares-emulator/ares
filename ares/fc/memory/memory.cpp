#include <fc/fc.hpp>

namespace ares::Famicom {

Bus bus;

//$0000-07ff = RAM (2KB)
//$0800-1fff = RAM (mirror)
//$2000-2007 = PPU
//$2008-3fff = PPU (mirror)
//$4000-4017 = APU + I/O
//$4020-403f = FDS
//$4018-ffff = Cartridge

auto Bus::read(uint16 address) -> uint8 {
  uint8 data = cartridge.readPRG(address);
       if(address <= 0x1fff) data = cpu.readRAM(address);
  else if(address <= 0x3fff) data = ppu.readIO(address);
  else if(address <= 0x4017) data = cpu.readIO(address);
  return data;
}

auto Bus::write(uint16 address, uint8 data) -> void {
  cartridge.writePRG(address, data);
  if(address <= 0x1fff) return cpu.writeRAM(address, data);
  if(address <= 0x3fff) return ppu.writeIO(address, data);
  if(address <= 0x4017) return cpu.writeIO(address, data);
}

}
