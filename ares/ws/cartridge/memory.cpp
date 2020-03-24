//20000-fffff
auto Cartridge::romRead(uint20 address) -> uint8 {
  if(!rom.data) return 0x00;
  uint28 offset;
  switch(address.byte(2)) {
  case 2:  offset = r.romBank0 << 16 | (uint16)address; break;  //20000-2ffff
  case 3:  offset = r.romBank1 << 16 | (uint16)address; break;  //30000-3ffff
  default: offset = r.romBank2 << 20 | (uint20)address; break;  //40000-fffff
  }
  return rom.data[offset & rom.mask];
}

auto Cartridge::romWrite(uint20 address, uint8 data) -> void {
}

//10000-1ffff
auto Cartridge::ramRead(uint20 address) -> uint8 {
  if(!ram.data) return 0x00;
  uint24 offset = r.sramBank << 16 | (uint16)address;
  return ram.data[offset & ram.mask];
}

auto Cartridge::ramWrite(uint20 address, uint8 data) -> void {
  if(!ram.data) return;
  uint24 offset = r.sramBank << 16 | (uint16)address;
  ram.data[offset & ram.mask] = data;
}
