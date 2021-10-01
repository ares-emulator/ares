auto MCD::readExternal(n1 upper, n1 lower, n22 address, n16 data) -> n16 {
  if(!MegaCD()) return data;
  address.bit(18,20) = 0;  //mirrors

  if(address >= 0x000000 && address <= 0x01ffff) {
    if(address == 0x70) return io.vectorLevel4 >> 16;
    if(address == 0x72) return io.vectorLevel4 >>  0;
    return bios[address >> 1];
  }

  if(address >= 0x020000 && address <= 0x03ffff) {
    address = io.pramBank << 17 | (n17)address;
    return pram[address >> 1];
  }

  if(address >= 0x200000 && address <= 0x23ffff) {
    if(io.wramMode == 0) {
    //if(io.wramSwitch == 1) return data;
      address = (n18)address >> 1;
    } else {
      if(address >= 0x220000) {
        if(address < 0x230000)
          // V32 cell-mapped window
          address = (address & 0x0FC00) >> 8 | (address & 0x003FC) << 6 | address & 0x10002;
        else if (address < 0x238000)
          // V16 cell-mapped window
          address = (address & 0x07E00) >> 7 | (address & 0x001FC) << 6 | address & 0x18002;
        else if (address < 0x23C000)
          // V8 cell-mapped window
          address = (address & 0x03F00) >> 6 | (address & 0x000FC) << 6 | address & 0x1C002;
        else
          // V4 cell-mapped window
          address = (address & 0x01F80) >> 5 | (address & 0x0007C) << 6 | address & 0x1E002;
      }
      address = (n17)address & ~1 | io.wramSelect;
    }
    if(!vdp.dma.active) return wram[address];

    //VDP DMA from Mega CD word RAM to VDP VRAM responds with a one-access delay
    //note: it is believed that the first transfer is the CPU prefetch, which isn't emulated here
    //games manually correct the first word transferred after VDP DMAs from word RAM
    data = io.wramLatch;
    io.wramLatch = wram[address];
    return data;
  }

  return data;
}

auto MCD::writeExternal(n1 upper, n1 lower, n22 address, n16 data) -> void {
  if(!MegaCD()) return;
  address.bit(18,20) = 0;  //mirrors

  if(address >= 0x020000 && address <= 0x03ffff) {
    address = io.pramBank << 17 | (n17)address;
    if(upper) pram[address >> 1].byte(1) = data.byte(1);
    if(lower) pram[address >> 1].byte(0) = data.byte(0);
    return;
  }

  if(address >= 0x200000 && address <= 0x23ffff) {
    if(io.wramMode == 0) {
    //if(io.wramSwitch == 1) return;
      address = (n18)address >> 1;
    } else {
      if(address >= 0x220000) {
        if(address < 0x230000)
          // V32 cell-mapped window
          address = (address & 0x0FC00) >> 8 | (address & 0x003FC) << 6 | address & 0x10002;
        else if (address < 0x238000)
          // V16 cell-mapped window
          address = (address & 0x07E00) >> 7 | (address & 0x001FC) << 6 | address & 0x18002;
        else if (address < 0x23C000)
          // V8 cell-mapped window
          address = (address & 0x03F00) >> 6 | (address & 0x000FC) << 6 | address & 0x1C002;
        else
          // V4 cell-mapped window
          address = (address & 0x01F80) >> 5 | (address & 0x0007C) << 6 | address & 0x1E002;
      }
      address = (n17)address & ~1 | io.wramSelect;
    }
    if(upper) wram[address].byte(1) = data.byte(1);
    if(lower) wram[address].byte(0) = data.byte(0);
    return;
  }
}
