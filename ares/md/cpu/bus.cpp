/* byte reads to word memory areas discard the unselected byte inside the M68K core.
 * as an optimization, the entire word is returned anyway.
 * byte writes to word memory areas that are addressable as bytes cannot enjoy this optimization.
 */

auto CPU::read(uint1 upper, uint1 lower, uint24 address, uint16 data) -> uint16 {
  if(address >= 0x000000 && address <= 0x3fffff) {
  //if(refresh.external >= 126) idle(min(2, 128 - refresh.external));
    if(!io.romEnable) return tmss[address >> 1];
    if(cartridge.bootable()) {
      if(cartridge.node) return cartridge.read(upper, lower, address, data);
    } else {
      if(expansion.node) return expansion.read(upper, lower, address, data);
    }
    return data;
  }

  if(address >= 0x400000 && address <= 0x7fffff) {
  //if(refresh.external >= 126) idle(min(2, 128 - refresh.external));
    if(!cartridge.bootable()) {
      if(cartridge.node) return cartridge.read(upper, lower, address, data);
    } else {
      if(expansion.node) return expansion.read(upper, lower, address, data);
    }
    return data;
  }

  if(address >= 0xa00000 && address <= 0xa0ffff) {
    if(!apu.granted()) return data;
    address.bit(15) = 0;  //a080000-a0ffff mirrors a00000-a07fff
    //word reads load the even input byte into both output bytes
    auto byte = apu.read(address | !upper);  //upper==0 only on odd byte reads
    return byte << 8 | byte << 0;
  }

  if(address >= 0xa10000 && address <= 0xbfffff) {
    if(cartridge.node) data = cartridge.readIO(upper, lower, address, data);
    if(expansion.node) data = expansion.readIO(upper, lower, address, data);
    data = readIO(upper, lower, address, data);
    return data;
  }

  if(address >= 0xc00000 && address <= 0xdfffff) {
    if(address.bit(5,7)) return cpu.ird();  //should deadlock the machine
    if(address.bit(16,18)) return cpu.ird();  //should deadlock the machine
    address.bit(8,15) = 0;  //mirrors
    if(address.bit(2,3) == 3) return cpu.ird();  //should return VDP open bus
    if(address.bit(4)) return cpu.ird();  //reading the PSG should deadlock the machine
    return vdp.read(address, data);
  }

  if(address >= 0xe00000 && address <= 0xffffff) {
  //if(refresh.ram >= 116) idle(min(3, 133 - refresh.ram));
    return ram[address >> 1];
  }

  return data;
}

auto CPU::write(uint1 upper, uint1 lower, uint24 address, uint16 data) -> void {
  if(address >= 0x000000 && address <= 0x3fffff) {
  //if(refresh.external >= 126) idle(min(2, 128 - refresh.external));
    if(cartridge.bootable()) {
      if(cartridge.node) return cartridge.write(upper, lower, address, data);
    } else {
      if(expansion.node) return expansion.write(upper, lower, address, data);
    }
    return;
  }

  if(address >= 0x400000 && address <= 0x7fffff) {
  //if(refresh.external >= 126) idle(min(2, 128 - refresh.external));
    if(!cartridge.bootable()) {
      if(cartridge.node) return cartridge.write(upper, lower, address, data);
    } else {
      if(expansion.node) return expansion.write(upper, lower, address, data);
    }
    return;
  }

  if(address >= 0xa00000 && address <= 0xa0ffff) {
    if(!apu.granted()) return;
    address.bit(15) = 0;  //a08000-a0ffff mirrors a00000-a07fff
    //word writes store the upper input byte into the lower output byte
    return apu.write(address | !upper, data.byte(upper));  //upper==0 only on odd byte reads
  }

  if(address >= 0xa10000 && address <= 0xbfffff) {
    if(cartridge.node) cartridge.writeIO(upper, lower, address, data);
    if(expansion.node) expansion.writeIO(upper, lower, address, data);
    writeIO(upper, lower, address, data);
    return;
  }

  if(address >= 0xc00000 && address <= 0xdfffff) {
    if(address.bit(5,7)) return;  //should deadlock the machine
    if(address.bit(16,18)) return;  //should deadlock the machine
    address.bit(8,15) = 0;  //mirrors
    if(address.bit(4)) {
      if(!lower) return;  //byte writes to even PSG registers has no effect
      return psg.write(data.byte(0));
    }
    return vdp.write(address, data);
  }

  if(address >= 0xe00000 && address <= 0xffffff) {
  //if(refresh.ram >= 116) idle(min(3, 133 - refresh.ram));
    if(upper) ram[address >> 1].byte(1) = data.byte(1);
    if(lower) ram[address >> 1].byte(0) = data.byte(0);
    return;
  }
}
