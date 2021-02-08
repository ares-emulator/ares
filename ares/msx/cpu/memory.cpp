auto CPU::read(n16 address) -> n8 {
  if(address == 0xffff) return readSecondarySlot();

  n2 page = address.bit(14,15);
  n2 primary = slot[page].primary;
  n2 secondary = slot[primary].secondary[page];

  if(primary == 0) {
    return rom.bios.read(address);
  }

  if(primary == 1) {
    return cartridge.read(address);
  }

  if(primary == 2) {
    return expansion.read(address);
  }

  if(primary == 3) {
    if(secondary == 0) {
      if(Model::MSX()) return ram.read(address);
      n22 logical = slot[page].memory << 14 | (n14)address;
      return ram.read(logical);
    }
    if(secondary == 1 && rom.sub) {
      return rom.sub.read(address);
    }
  }

  return 0xff;
}

auto CPU::write(n16 address, n8 data) -> void {
  if(address == 0xffff) return writeSecondarySlot(data);

  n2 page = address.bit(14,15);
  n2 primary = slot[page].primary;
  n2 secondary = slot[primary].secondary[page];

  if(primary == 0) {
    return;
  }

  if(primary == 1) {
    return cartridge.write(address, data);
  }

  if(primary == 2) {
    return expansion.write(address, data);
  }

  if(primary == 3) {
    if(Model::MSX()) return ram.write(address, data);
    n22 logical = slot[page].memory << 14 | (n14)address;
    return ram.write(logical, data);
  }
}

//

auto CPU::in(n16 address) -> n8 {
  switch((n8)address) {
  case 0x98: return vdp.read(0);
  case 0x99: return vdp.read(1);
  case 0x9a: return vdp.read(2);
  case 0x9b: return vdp.read(3);
  case 0xa2: return psg.read();
  case 0xa8: return readPrimarySlot();
  case 0xa9: return keyboard.read();
  case 0xfc: case 0xfd: case 0xfe: case 0xff:
    if(Model::MSX()) return 0xff;
    //note: reading these ports is said to be unreliable;
    //but since it's not specified how so, that is not emulated here
    return slot[(n2)address].memory;
  }
  return 0xff;
}

auto CPU::out(n16 address, n8 data) -> void {
  switch((n8)address) {
  case 0x98: return vdp.write(0, data);
  case 0x99: return vdp.write(1, data);
  case 0x9a: return vdp.write(2, data);
  case 0x9b: return vdp.write(3, data);
  case 0xa0: return psg.select(data);
  case 0xa1: return psg.write(data);
  case 0xa8: return writePrimarySlot(data);
  case 0xaa: return keyboard.write(data.bit(0,3));
  case 0xfc: case 0xfd: case 0xfe: case 0xff:
    slot[(n2)address].memory = data;
    return;
  }
}

//

auto CPU::readPrimarySlot() -> n8 {
  n8 data;
  data.bit(0,1) = slot[0].primary;
  data.bit(2,3) = slot[1].primary;
  data.bit(4,5) = slot[2].primary;
  data.bit(6,7) = slot[3].primary;
  return data;
}

auto CPU::writePrimarySlot(n8 data) -> void {
  slot[0].primary = data.bit(0,1);
  slot[1].primary = data.bit(2,3);
  slot[2].primary = data.bit(4,5);
  slot[3].primary = data.bit(6,7);
}

//

auto CPU::readSecondarySlot() -> n8 {
  auto primary = slot[3].primary;
  n8 data;
  data.bit(0,1) = slot[primary].secondary[0];
  data.bit(2,3) = slot[primary].secondary[1];
  data.bit(4,5) = slot[primary].secondary[2];
  data.bit(6,7) = slot[primary].secondary[3];
  return ~data;
}

auto CPU::writeSecondarySlot(n8 data) -> void {
  auto primary = slot[3].primary;
  slot[primary].secondary[0] = data.bit(0,1);
  slot[primary].secondary[1] = data.bit(2,3);
  slot[primary].secondary[2] = data.bit(4,5);
  slot[primary].secondary[3] = data.bit(6,7);
}
