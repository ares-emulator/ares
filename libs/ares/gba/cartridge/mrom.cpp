auto Cartridge::MROM::read(n32 address) -> n16 {
  //out-of-bounds accesses
  if(address >= size) {
    if(!mirror) return pageAddr;
    address &= (size - 1);
  }

  return data[address >> 1];
}

auto Cartridge::MROM::write(n32 address, n16 half) -> void {
}

auto Cartridge::MROM::burstAddr(n32 address) -> n32 {
  //end burst transfer if requesting last address on page
  if((address & 0x1fffe) == 0x1fffe) burst = false;

  //use latched lower 16 bits of address lines
  n8 page = address >> 17;
  return (page << 17) | (pageAddr << 1);
}
