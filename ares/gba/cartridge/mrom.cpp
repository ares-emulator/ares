auto Cartridge::MROM::read(uint mode, uint32 address) -> uint32 {
  if(mode & Word) {
    uint32 word = 0;
    word |= read(mode & ~Word | Half, (address & ~3) + 0) <<  0;
    word |= read(mode & ~Word | Half, (address & ~3) + 2) << 16;
    return word;
  }

  address &= 0x01ff'ffff;
  if(address >= size) return (uint16)(address >> 1);

  if(mode & Half) address &= ~1;
  auto p = data + address;
  if(mode & Half) return p[0] << 0 | p[1] << 8;
  if(mode & Byte) return p[0] << 0;
  return 0;  //should never occur
}

auto Cartridge::MROM::write(uint mode, uint32 address, uint32 word) -> void {
}
