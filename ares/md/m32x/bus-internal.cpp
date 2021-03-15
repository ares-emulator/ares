auto M32X::readInternal(n1 upper, n1 lower, n32 address, n16 data) -> n16 {
  if(address >= 0x2000'0000 && address <= 0x3fff'ffff) {
    address -= 0x2000'0000;
  }

  if(address >= 0x0000'0000 && address <= 0x0000'3fff) {
    if(shm.active()) return shm.bootROM[address >> 1];
    if(shs.active()) return shs.bootROM[address >> 1];
  }

  if(address >= 0x0000'4000 && address <= 0x0000'43ff) {
    return readInternalIO(upper, lower, address, data);
  }

  if(address >= 0x0200'0000 && address <= 0x023f'ffff) {
    return rom[address >> 1];
  }

  if(address >= 0x0400'0000 && address <= 0x0403'ffff) {
    return dram[address >> 1];
  }

  if(address >= 0x0600'0000 && address <= 0x0603'ffff) {
    return sdram[address >> 1];
  }

  if(address >= 0xc000'0000 && address <= 0xc000'0fff) {
    if(shm.active()) return shm.cache[address >> 1 & 0x7ff];
    if(shs.active()) return shs.cache[address >> 1 & 0x7ff];
  }

  //SH2 internal registers
  if(address >= 0xffff'fe00 && address <= 0xffff'ffff) {
  //print("* r", hex(address, 8L), "\n");
  }

  return data;
}

auto M32X::writeInternal(n1 upper, n1 lower, n32 address, n16 data) -> void {
  if(address >= 0x2000'0000 && address <= 0x3fff'ffff) {
    address -= 0x2000'0000;
  }

  if(address >= 0x0000'4000 && address <= 0x0000'43ff) {
    return writeInternalIO(upper, lower, address, data);
  }

  if(address >= 0x0400'0000 && address <= 0x0401'ffff) {
    if(upper) dram[address >> 1 & 0xffff].byte(1) = data.byte(1);
    if(lower) dram[address >> 1 & 0xffff].byte(0) = data.byte(0);
    return;
  }

  if(address >= 0x0402'0000 && address <= 0x0403'ffff) {
    if(upper && data.byte(1)) dram[address >> 1 & 0xffff].byte(1) = data.byte(1);
    if(lower && data.byte(0)) dram[address >> 1 & 0xffff].byte(0) = data.byte(0);
    return;
  }

  if(address >= 0x0600'0000 && address <= 0x0603'ffff) {
    if(upper) sdram[address >> 1].byte(1) = data.byte(1);
    if(lower) sdram[address >> 1].byte(0) = data.byte(0);
    return;
  }

  if(address >= 0xc000'0000 && address <= 0xc000'0fff) {
    if(shm.active()) {
      if(upper) shm.cache[address >> 1 & 0x7ff].byte(1) = data.byte(1);
      if(lower) shm.cache[address >> 1 & 0x7ff].byte(0) = data.byte(0);
    }
    if(shs.active()) {
      if(upper) shs.cache[address >> 1 & 0x7ff].byte(1) = data.byte(1);
      if(lower) shs.cache[address >> 1 & 0x7ff].byte(0) = data.byte(0);
    }
  }

  //SH2 internal registers
  if(address >= 0xffff'fe00 && address <= 0xffff'ffff) {
  //print("* w", hex(address, 8L), " = ", hex(data, 4L), "\n");
  }
}
