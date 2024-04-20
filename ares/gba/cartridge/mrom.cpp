auto Cartridge::MROM::read(u32 mode, n32 address) -> n32 {
  if(mode & Word) {
    n32 word = 0;
    word |= readBus(mode & ~Word | Half, (address & ~3) + 0) <<  0;
    word |= readBus(mode & ~Word | Half, (address & ~3) + 2) << 16;
    return word;
  }

  return readBus(mode, address);
}

auto Cartridge::MROM::readBus(u32 mode, n32 address) -> n16 {
  address &= 0x01ff'ffff;

  if(address >= size) {
    if(!mirror) return (n16)(address >> 1);
    address &= size - 1;
  }

  if(mode & Half) address &= ~1;
  auto p = data + address;
  if(mode & Half) return p[0] << 0 | p[1] << 8;
  if(mode & Byte) return p[0] << 0;
  return 0;  //should never occur
}

auto Cartridge::MROM::write(u32 mode, n32 address, n32 word) -> void {
}
