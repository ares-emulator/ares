inline auto Bus::read(n1 upper, n1 lower, n24 address, n16 data) -> n16 {
  if(address >= 0x000000 && address <= 0x3fffff) {
    waitRefreshExternal();
    if(!cpu.io.romEnable) {
      data = cpu.tmss[address >> 1];
    } else if(cartridge.bootable()) {
      data = cartridge.read(upper, lower, address, data);
    } else {
      data = mcd.readExternal(upper, lower, address, data);
    }
    return data;
  }

  if(address >= 0x400000 && address <= 0x7fffff) {
    waitRefreshExternal();
    if(!cartridge.bootable()) {
      data = cartridge.read(upper, lower, address, data);
    } else {
      data = mcd.readExternal(upper, lower, address, data);
    }
    return data;
  }

  if(address >= 0x800000 && address <= 0x9fffff) {
    data = m32x.readExternal(upper, lower, address, data);
    return data;
  }

  if(address >= 0xa00000 && address <= 0xa0ffff) {
    if(!apu.busgrantedCPU()) return data;
    if(cpu.active()) cpu.wait(3);  //todo: inaccurate approximation of Z80 RAM access delay
    address.bit(15) = 0;  //a080000-a0ffff mirrors a00000-a07fff
    //word reads load the even input byte into both output bytes
    auto byte = apu.read(address | !upper);  //upper==0 only on odd byte reads
    return byte << 8 | byte << 0;
  }

  if(address >= 0xa10000 && address <= 0xbfffff) {
    data = cartridge.readIO(upper, lower, address, data);
    data = mcd.readExternalIO(upper, lower, address, data);
    data = cpu.readIO(upper, lower, address, data);
    return data;
  }

  if(address >= 0xc00000 && address <= 0xdfffff) {
    if(address.bit(5,7)) return cpu.ird();  //should deadlock the machine
    if(address.bit(16,18)) return cpu.ird();  //should deadlock the machine
    address.bit(8,15) = 0;  //mirrors
    if(address.bit(2,3) == 3) return cpu.ird();  //should return VDP open bus
    return vdp.read(upper, lower, address, data);
  }

  if(address >= 0xe00000 && address <= 0xffffff) {
    waitRefreshRAM();
    return cpu.ram[address >> 1];
  }

  return data;
}

inline auto Bus::write(n1 upper, n1 lower, n24 address, n16 data) -> void {
  if(address >= 0x000000 && address <= 0x3fffff) {
    waitRefreshExternal();
    if(cartridge.bootable()) {
      cartridge.write(upper, lower, address, data);
    } else {
      mcd.writeExternal(upper, lower, address, data);
    }
    return;
  }

  if(address >= 0x400000 && address <= 0x7fffff) {
    waitRefreshExternal();
    if(!cartridge.bootable()) {
      cartridge.write(upper, lower, address, data);
    } else {
      mcd.writeExternal(upper, lower, address, data);
    }
    return;
  }

  if(address >= 0x800000 && address <= 0x9fffff) {
    m32x.writeExternal(upper, lower, address, data);
    return;
  }

  if(address >= 0xa00000 && address <= 0xa0ffff) {
    if(!apu.busgrantedCPU()) return;
    if(cpu.active()) cpu.wait(3);  //todo: inaccurate approximation of Z80 RAM access delay
    address.bit(15) = 0;  //a08000-a0ffff mirrors a00000-a07fff
    //word writes store the upper input byte into the lower output byte
    return apu.write(address | !upper, data.byte(upper));  //upper==0 only on odd byte reads
  }

  if(address >= 0xa10000 && address <= 0xbfffff) {
    cartridge.writeIO(upper, lower, address, data);
    mcd.writeExternalIO(upper, lower, address, data);
    cpu.writeIO(upper, lower, address, data);
    return;
  }

  if(address >= 0xc00000 && address <= 0xdfffff) {
    if(address.bit(5,7)) return;  //should deadlock the machine
    if(address.bit(16,18)) return;  //should deadlock the machine
    address.bit(8,15) = 0;  //mirrors
    return vdp.write(upper, lower, address, data);
  }

  if(address >= 0xe00000 && address <= 0xffffff) {
    waitRefreshRAM();
    if(upper) cpu.ram[address >> 1].byte(1) = data.byte(1);
    if(lower) cpu.ram[address >> 1].byte(0) = data.byte(0);
    return;
  }
}

//0-127
inline auto Bus::waitRefreshExternal() -> void {
  while(cpu.refresh.external >= 126) {
    if(cpu.active()) cpu.wait(1);
    if(scheduler.synchronizing()) break;
    if(apu.active()) apu.step(1);
    if(vdp.active()) break;
  }
}

//0-132
inline auto Bus::waitRefreshRAM() -> void {
  while(cpu.refresh.ram >= 130) {
    if(cpu.active()) cpu.wait(1);
    if(scheduler.synchronizing()) break;
    if(apu.active()) apu.step(1);
    if(vdp.active()) break;
  }
}
