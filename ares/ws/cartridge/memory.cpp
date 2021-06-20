//20000-fffff
auto Cartridge::readROM(n20 address) -> n8 {
  if(!rom) return 0x00;
  n28 offset;
  switch(address.bit(16,19)) {
  case 2:  offset = io.romBank0 << 16 | (n16)address; break;  //20000-2ffff
  case 3:  offset = io.romBank1 << 16 | (n16)address; break;  //30000-3ffff
  default: offset = io.romBank2 << 20 | (n20)address; break;  //40000-fffff
  }
  return rom.read(offset);
}

auto Cartridge::writeROM(n20 address, n8 data) -> void {
}

//10000-1ffff
auto Cartridge::readRAM(n20 address) -> n8 {
  if(!ram) return 0x00;
  n24 offset = io.sramBank << 16 | (n16)address;
  return ram.read(offset);
}

auto Cartridge::writeRAM(n20 address, n8 data) -> void {
  if(!ram) return;
  n24 offset = io.sramBank << 16 | (n16)address;
  ram.write(offset, data);
}
