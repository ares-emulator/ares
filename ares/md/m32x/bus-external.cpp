auto M32X::readExternal(n1 upper, n1 lower, n24 address, n16 data) -> n16 {
  if(!io.adapterEnable) {
    return data = rom[address >> 1];
  }

  if(address >= 0x000000 && address <= 0x0000ff) {
    return data = vectors[address >> 1];
  }

  if(address >= 0x840000 && address <= 0x85ffff) {
    return data = dram[address >> 1];
  }

  if(address >= 0x880000 && address <= 0x8fffff) {
    return data = rom[address >> 1 & 0x3ffff];
  }

  if(address >= 0x900000 && address <= 0x9fffff) {
    //...
  }

  return 0;
}

auto M32X::writeExternal(n1 upper, n1 lower, n24 address, n16 data) -> void {
  if(!io.adapterEnable) {
    return;
  }

  if(address >= 0x840000 && address <= 0x85ffff) {
    if(upper) dram[address >> 1].byte(1) = data.byte(1);
    if(lower) dram[address >> 1].byte(0) = data.byte(0);
    return;
  }
}
