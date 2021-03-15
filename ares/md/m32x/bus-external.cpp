auto M32X::readExternal(n1 upper, n1 lower, n24 address, n16 data) -> n16 {
  if(!io.adapterEnable) {
    return rom[address >> 1];
  }

  if(address >= 0x000000 && address <= 0x0000ff) {
    if(address == 0x70) return io.vectorLevel4 >> 16;
    if(address == 0x72) return io.vectorLevel4 >>  0;
    return vectors[address >> 1];
  }

  if(address >= 0x840000 && address <= 0x87ffff) {
    return dram[address >> 1];
  }

  if(address >= 0x880000 && address <= 0x8fffff) {
    return rom[address >> 1 & 0x3ffff];
  }

  if(address >= 0x900000 && address <= 0x9fffff) {
    address = io.romBank * 0x100000 | address & 0x0fffff;
    return rom[address >> 1];
  }

  return data;
}

auto M32X::writeExternal(n1 upper, n1 lower, n24 address, n16 data) -> void {
  if(!io.adapterEnable) {
    return;
  }

  if(address >= 0x000000 && address <= 0x0000ff) {
    if(address == 0x70 && upper) io.vectorLevel4.byte(3) = data.byte(1);
    if(address == 0x70 && lower) io.vectorLevel4.byte(2) = data.byte(0);
    if(address == 0x72 && upper) io.vectorLevel4.byte(1) = data.byte(1);
    if(address == 0x72 && lower) io.vectorLevel4.byte(0) = data.byte(0);
    return;
  }

  if(address >= 0x840000 && address <= 0x85ffff) {
    if(upper) dram[address >> 1 & 0xffff].byte(1) = data.byte(1);
    if(lower) dram[address >> 1 & 0xffff].byte(0) = data.byte(0);
    return;
  }

  if(address >= 0x860000 && address <= 0x87ffff) {
    if(upper && data.byte(1)) dram[address >> 1 & 0xffff].byte(1) = data.byte(1);
    if(lower && data.byte(0)) dram[address >> 1 & 0xffff].byte(0) = data.byte(0);
    return;
  }
}
