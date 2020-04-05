auto SuperFX::read(uint24 address, uint8 data) -> uint8 {
  if((address & 0xc00000) == 0x000000) {  //$00-3f:0000-7fff,:8000-ffff
    while(!regs.scmr.ron) {
      step(6);
      synchronize(cpu);
      if(scheduler.synchronizing()) break;
    }
    return rom.read((((address & 0x3f0000) >> 1) | (address & 0x7fff)) & romMask);
  }

  if((address & 0xe00000) == 0x400000) {  //$40-5f:0000-ffff
    while(!regs.scmr.ron) {
      step(6);
      synchronize(cpu);
      if(scheduler.synchronizing()) break;
    }
    return rom.read(address & romMask);
  }

  if((address & 0xfe0000) == 0x700000) {  //$70-71:0000-ffff
    while(!regs.scmr.ran) {
      step(6);
      synchronize(cpu);
      if(scheduler.synchronizing()) break;
    }
    return ram.read(address & ramMask);
  }

  return data;
}

auto SuperFX::write(uint24 address, uint8 data) -> void {
  if((address & 0xfe0000) == 0x700000) {  //$70-71:0000-ffff
    while(!regs.scmr.ran) {
      step(6);
      synchronize(cpu);
      if(scheduler.synchronizing()) break;
    }
    return ram.write(address & ramMask, data);
  }
}

auto SuperFX::readOpcode(uint16 address) -> uint8 {
  uint16 offset = address - regs.cbr;
  if(offset < 512) {
    if(cache.valid[offset >> 4] == false) {
      uint dp = offset & 0xfff0;
      uint sp = (regs.pbr << 16) + ((regs.cbr + dp) & 0xfff0);
      for(uint n : range(16)) {
        step(regs.clsr ? 5 : 6);
        cache.buffer[dp++] = read(sp++);
      }
      cache.valid[offset >> 4] = true;
    } else {
      step(regs.clsr ? 1 : 2);
    }
    return cache.buffer[offset];
  }

  if(regs.pbr <= 0x5f) {
    //$00-5f:0000-ffff ROM
    syncROMBuffer();
    step(regs.clsr ? 5 : 6);
    return read(regs.pbr << 16 | address);
  } else {
    //$60-7f:0000-ffff RAM
    syncRAMBuffer();
    step(regs.clsr ? 5 : 6);
    return read(regs.pbr << 16 | address);
  }
}

inline auto SuperFX::peekpipe() -> uint8 {
  uint8 result = regs.pipeline;
  regs.pipeline = readOpcode(regs.r[15]);
  regs.r[15].modified = false;
  return result;
}

inline auto SuperFX::pipe() -> uint8 {
  uint8 result = regs.pipeline;
  regs.pipeline = readOpcode(++regs.r[15]);
  regs.r[15].modified = false;
  return result;
}

auto SuperFX::flushCache() -> void {
  for(uint n : range(32)) cache.valid[n] = false;
}

auto SuperFX::readCache(uint16 address) -> uint8 {
  address = (address + regs.cbr) & 511;
  return cache.buffer[address];
}

auto SuperFX::writeCache(uint16 address, uint8 data) -> void {
  address = (address + regs.cbr) & 511;
  cache.buffer[address] = data;
  if((address & 15) == 15) cache.valid[address >> 4] = true;
}
