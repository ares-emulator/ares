//20000-fffff
auto Cartridge::romRead(n20 address) -> n8 {
  if(!rom) return 0x00;
  n28 offset;
  switch(address.byte(2)) {
  case 2:  offset = r.romBank0 << 16 | (n16)address; break;  //20000-2ffff
  case 3:  offset = r.romBank1 << 16 | (n16)address; break;  //30000-3ffff
  default: offset = r.romBank2 << 20 | (n20)address; break;  //40000-fffff
  }
  return rom.read(offset);
}

auto Cartridge::romWrite(n20 address, n8 data) -> void {
}

//10000-1ffff
auto Cartridge::ramRead(n20 address) -> n8 {
  if(!ram) return 0x00;
  n24 offset = r.sramBank << 16 | (n16)address;
  return ram.read(offset);
}

auto Cartridge::ramWrite(n20 address, n8 data) -> void {
  if(!ram) return;
  n24 offset = r.sramBank << 16 | (n16)address;
  ram.write(offset, data);
}
