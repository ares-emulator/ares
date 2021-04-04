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
      address = (n18)address;
    } else {
      address = !io.wramSelect << 17 | (n17)address;
    }
    if(!vdp.dma.active) return wram[address >> 1];

    //VDP DMA from Mega CD word RAM to VDP VRAM responds with a one-access delay
    //note: it is believed that the first transfer is the CPU prefetch, which isn't emulated here
    //games manually correct the first word transferred after VDP DMAs from word RAM
    data = io.wramLatch;
    io.wramLatch = wram[address >> 1];
    return data;
  }

  return data;
}

auto MCD::writeExternal(n1 upper, n1 lower, n22 address, n16 data) -> void {
  if(!MegaCD()) return;
  address.bit(18,20) = 0;  //mirrors

  if(address >= 0x000000 && address <= 0x01ffff) {
    if(address == 0x70 && upper) io.vectorLevel4.byte(3) = data.byte(1);
    if(address == 0x70 && lower) io.vectorLevel4.byte(2) = data.byte(0);
    if(address == 0x72 && upper) io.vectorLevel4.byte(1) = data.byte(1);
    if(address == 0x72 && lower) io.vectorLevel4.byte(0) = data.byte(0);
    return;
  }

  if(address >= 0x020000 && address <= 0x03ffff) {
    address = io.pramBank << 17 | (n17)address;
    if(upper) pram[address >> 1].byte(1) = data.byte(1);
    if(lower) pram[address >> 1].byte(0) = data.byte(0);
    return;
  }

  if(address >= 0x200000 && address <= 0x23ffff) {
    if(io.wramMode == 0) {
    //if(io.wramSwitch == 1) return;
      address = (n18)address;
    } else {
      address = !io.wramSelect << 17 | (n17)address;
    }
    if(upper) wram[address >> 1].byte(1) = data.byte(1);
    if(lower) wram[address >> 1].byte(0) = data.byte(0);
    return;
  }
}
