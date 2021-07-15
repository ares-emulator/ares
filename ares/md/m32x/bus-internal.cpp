auto M32X::readInternal(n1 upper, n1 lower, n32 address, n16 data) -> n16 {
  if(address >= 0x0000'0000 && address <= 0x0000'3fff) {
    if(shm.active()) return shm.bootROM[address >> 1];
    if(shs.active()) return shs.bootROM[address >> 1];
  }

  if(address >= 0x0000'4000 && address <= 0x0000'43ff) {
    return readInternalIO(upper, lower, address, data);
  }

  if(address >= 0x0200'0000 && address <= 0x023f'ffff) {
    while(dreq.vram) {
      // SH2 ROM accesses stall while RV is set
      if(shm.active()) shm.step(1);
      if(shs.active()) shs.step(1);
    }

    return rom[address >> 1];
  }

  if(address >= 0x0400'0000 && address <= 0x0403'ffff) {
    return vdp.bbram[address >> 1 & 0xffff];
  }

  if(address >= 0x0600'0000 && address <= 0x0603'ffff) {
    return sdram[address >> 1 & 0x1ffff];
  }

  return data;
}

auto M32X::writeInternal(n1 upper, n1 lower, n32 address, n16 data) -> void {
  if(address >= 0x0000'4000 && address <= 0x0000'43ff) {
    return writeInternalIO(upper, lower, address, data);
  }

  if(address >= 0x0400'0000 && address <= 0x0401'ffff) {
    if (!vdp.framebufferAccess) return;
    if(!data && (!upper || !lower)) return;  //8-bit 0x00 writes do not go through
    if(upper) vdp.bbram[address >> 1 & 0xffff].byte(1) = data.byte(1);
    if(lower) vdp.bbram[address >> 1 & 0xffff].byte(0) = data.byte(0);
    return;
  }

  if(address >= 0x0402'0000 && address <= 0x0403'ffff) {
    if (!vdp.framebufferAccess) return;
    if(upper && data.byte(1)) vdp.bbram[address >> 1 & 0xffff].byte(1) = data.byte(1);
    if(lower && data.byte(0)) vdp.bbram[address >> 1 & 0xffff].byte(0) = data.byte(0);
    return;
  }

  if(address >= 0x0600'0000 && address <= 0x0603'ffff) {
    if(upper) sdram[address >> 1 & 0x1ffff].byte(1) = data.byte(1);
    if(lower) sdram[address >> 1 & 0x1ffff].byte(0) = data.byte(0);
    return;
  }
}
