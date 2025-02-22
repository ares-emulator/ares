auto Cartridge::MROM::read(u32 mode, n32 address) -> n16 {
  //out-of-bounds accesses
  if(address >= size) {
    if(!mirror) return pageAddr;
    address &= (size - 1);
  }

  return data[address >> 1];
}

auto Cartridge::MROM::write(u32 mode, n32 address, n16 half) -> void {
}

auto Cartridge::MROM::burstAddr(u32 mode, n32 address) -> n32 {
  //use latched lower 16 bits of address lines if sequential
  n8 page = address >> 17;
  if(mode & Nonsequential) {
    pageAddr = address >> 1;
    burst = true;
  }

  //end burst transfer if requesting last address on page
  if((address & 0x1fffe) == 0x1fffe) burst = false;

  return (page << 17) | (pageAddr << 1);
}
