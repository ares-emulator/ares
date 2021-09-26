auto MCD::read(n1 upper, n1 lower, n24 address, n16 data) -> n16 {
  address.bit(20,23) = 0;  //mirroring

  if(address >= 0x000000 && address <= 0x07ffff) {
    return pram[address >> 1];
  }

  if(address >= 0x080000 && address <= 0x0bffff) {
    if(io.wramMode == 0) {
    //if(io.wramSwitch == 0) return data;
      address = (n18)address >> 1;
      return wram[address];
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

  if(address >= 0x0e0000 && address <= 0x0effff) {
    if(!lower) return data;
    return bram.read(address >> 1);
  }

  if(address >= 0x0f0000 && address <= 0x0f7fff) {
    if(!lower) return data;
    return pcm.read(address >> 1, data);
  }

  if(address >= 0x0f8000 && address <= 0x0fffff) {
    return readIO(upper, lower, address, data);
  }

  return data;  //unreachable
}

auto MCD::write(n1 upper, n1 lower, n24 address, n16 data) -> void {
  address.bit(20,23) = 0;  //mirroring

  if(address >= 0x000000 && address <= 0x07ffff) {
    if(upper) pram[address >> 1].byte(1) = data.byte(1);
    if(lower) pram[address >> 1].byte(0) = data.byte(0);
    return;
  }

  if(address >= 0x080000 && address <= 0x0bffff) {
    if(io.wramMode == 0) {
    //if(io.wramSwitch == 0) return;
      address = (n18)address >> 1;
      if(upper) wram[address].byte(1) = data.byte(1);
      if(lower) wram[address].byte(0) = data.byte(0);
    }
  }

  if(address >= 0x0c0000 && address <= 0x0dffff) {
    if(io.wramMode == 1) {
      address = (n17)address & ~1 | !io.wramSelect;
      if(upper) wram[address].byte(1) = data.byte(1);
      if(lower) wram[address].byte(0) = data.byte(0);
    }
  }

  if(address >= 0x0e0000 && address <= 0x0effff) {
    if(!lower) return;
    return bram.write(address >> 1, data);
  }

  if(address >= 0x0f0000 && address <= 0x0f7fff) {
    if(!lower) return;
    return pcm.write(address >> 1, data);
  }

  if(address >= 0x0f8000 && address <= 0x0fffff) {
    return writeIO(upper, lower, address, data);
  }

  return;  //unreachable
}
