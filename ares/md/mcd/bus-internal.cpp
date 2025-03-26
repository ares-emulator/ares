auto MCD::read(n1 upper, n1 lower, n24 address, n16 data) -> n16 {
  if(address >= 0x000000 && address <= 0x07ffff) {
    return pram[address >> 1];
  }

  if(address >= 0x080000 && address <= 0x0bffff) {
    if(io.wramMode == 0) {
      if(io.wramSwitch == 0) return data;
      address = (n18)address >> 1;
      return wram[address];
    } else {
      // dot-mapped window
      n4 ofs = address.bit(1) ? 0 : 8; // low/high byte selector
      address = (n17)(address >> 1) & ~1 | !io.wramSelect;
      return wram[address].bit(4+ofs,7+ofs) << 8 | wram[address].bit(0+ofs,3+ofs);
    }
    return data;
  }

  if(address >= 0x0c0000 && address <= 0x0dffff) {
    if(io.wramMode == 1) {
      address = (n17)address & ~1 | !io.wramSelect;
      return wram[address];
    }
    return data;
  }

  if(MegaLD() && address >= 0xfd0000 && address <= 0xfdffff) {
    if (!lower) return data;
    return ld.read(address | 0x1);
  }

  if(address >= 0xfe0000 && address <= 0xfeffff) {
    if(!lower) return data;
    return bram.read(address >> 1);
  }

  if(address >= 0xff0000 && address <= 0xff7fff) {
    if(!lower) return data;
    return pcm.read(address >> 1, data);
  }

  if(address >= 0xff8000 && address <= 0xffffff) {
    return readIO(upper, lower, address, data);
  }

  return data;  //unreachable
}

auto MCD::write(n1 upper, n1 lower, n24 address, n16 data) -> void {
  if(address >= 0x000000 && address <= 0x07ffff
  && address >= (n24)io.pramProtect << 9) {
    if(upper) pram[address >> 1].byte(1) = data.byte(1);
    if(lower) pram[address >> 1].byte(0) = data.byte(0);
    return;
  }

  if(address >= 0x080000 && address <= 0x0bffff) {
    if(io.wramMode == 0) {
      while(io.wramSwitch == 0) { // wordram is unavailable, hold for !DTACK
        wait(7); // arbitrary wait (~4 maincpu ticks)
        if(io.halt) return;
      }
      address = (n18)address >> 1;
      if(upper) wram[address].byte(1) = data.byte(1);
      if(lower) wram[address].byte(0) = data.byte(0);
    } else {
      // dot-mapped window
      n4 ofs = address.bit(1) ? 0 : 8; // low/high byte selector
      address = (n17)(address >> 1) & ~1 | !io.wramSelect;
      switch(mcd.io.wramPriority) {
      case 0: // normal write
        if(upper) wram[address].bit(4+ofs,7+ofs) = data.bit(8,11);
        if(lower) wram[address].bit(0+ofs,3+ofs) = data.bit(0,3);
        return;
      case 1: // under write
        if(upper && !wram[address].bit(4+ofs,7+ofs))
          wram[address].bit(4+ofs,7+ofs) = data.bit(8,11);
        if(lower && !wram[address].bit(0+ofs,3+ofs))
          wram[address].bit(0+ofs,3+ofs) = data.bit(0,3);
        return;
      case 2: // over write
        if(upper && data.bit(8,11))
          wram[address].bit(4+ofs,7+ofs) = data.bit(8,11);
        if(lower && data.bit(0,3))
          wram[address].bit(0+ofs,3+ofs) = data.bit(0,3);
        return;
      }
      return; // invalid priority mode
    }
  }

  if(address >= 0x0c0000 && address <= 0x0dffff) {
    if(io.wramMode == 1) {
      address = (n17)address & ~1 | !io.wramSelect;
      if(upper) wram[address].byte(1) = data.byte(1);
      if(lower) wram[address].byte(0) = data.byte(0);
    }
  }

  if(MegaLD() && address >= 0xfd0000 && address <= 0xfdffff) {
    if (!lower) return;
    return ld.write(address | 0x1, data);
  }

  if(address >= 0xfe0000 && address <= 0xfeffff) {
    if(!lower) return;
    return bram.write(address >> 1, data);
  }

  if(address >= 0xff0000 && address <= 0xff7fff) {
    if(!lower) return;
    return pcm.write(address >> 1, data);
  }

  if(address >= 0xff8000 && address <= 0xffffff) {
    return writeIO(upper, lower, address, data);
  }

  return;  //unreachable
}
