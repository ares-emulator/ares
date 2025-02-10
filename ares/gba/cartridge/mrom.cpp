auto Cartridge::MROM::read(u32 mode, n32 address) -> n16 {
  address &= 0x01ff'ffff;

  if(address >= size) {
    if(!mirror) return (n16)(address >> 1);
    address &= size - 1;
  }

  if(mode & Half) address &= ~1;
  auto p = data + address;
  if(mode & Half) return p[0] << 0 | p[1] << 8;
  return p[0] << 0;
}

auto Cartridge::MROM::write(u32 mode, n32 address, n32 word) -> void {
}
