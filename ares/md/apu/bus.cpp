/* the APU can write to CPU RAM, but it cannot read from CPU RAM:
 * the exact returned value varies per system, but it always fails.
 * it is unknown which other regions of the bus are inaccessible to the APU.
 * it would certainly go very badly if the APU could reference itself at $a0xxxx.
 * for now, assume that only the cartridge and expansion buses are also accessible.
 */

auto APU::read(n16 address) -> n8 {
  switch(address) {
  case 0x0000 ... 0x3fff: return ram.read(address);  //$2000-3fff mirrors $0000-1fff
  case 0x4000 ... 0x5fff: return opn2.readStatus();
  case 0x7f00 ... 0x7fff: return readExternal(0xc00000 | (n8)address);
  case 0x8000 ... 0xffff: return readExternal(state.bank << 15 | (n15)address);
  default:
    debug(unusual, "[APU] read(0x", hex(address, 4L), ")");
    return 0x00;
  }
}

auto APU::write(n16 address, n8 data) -> void {
  switch(address) {
  case 0x0000 ... 0x3fff: return ram.write(address, data);  //$2000-3fff mirrors $0000-1fff
  case 0x4000 ... 0x5fff:
    switch(0x4000 | address & 3) {
    case 0x4000: return opn2.writeAddress(0 << 8 | data);
    case 0x4001: return opn2.writeData(data);
    case 0x4002: return opn2.writeAddress(1 << 8 | data);
    case 0x4003: return opn2.writeData(data);
    }
  case 0x6000 ... 0x60ff: return (void)(state.bank = data.bit(0) << 8 | state.bank >> 1);
  case 0x7f00 ... 0x7fff: return writeExternal(0xc00000 | (n8)address, data);
  case 0x8000 ... 0xffff: return writeExternal(state.bank << 15 | (n15)address, data);
  default:
    debug(unusual, "[APU] write(0x", hex(address, 4L), ")");
    return;
  }
}

auto APU::readExternal(n24 address) -> n8 {
  //bus arbiter delay rough approximation
  step(3);
  while(MegaDrive::bus.acquired() && !scheduler.synchronizing()) step(1);
  MegaDrive::bus.acquire(MegaDrive::Bus::APU);

  n8 data = 0xff;
  switch(address) {
  case 0x000000 ... 0x9fffff:
  case 0xa10000 ... 0xa1ffff:
  case 0xc00000 ... 0xc000ff:
    if(address & 1) {
      data = MegaDrive::bus.read(0, 1, address & ~1, 0x00).byte(0);
    } else {
      data = MegaDrive::bus.read(1, 0, address & ~1, 0x00).byte(1);
    }
    break;
  default:
    debug(unusual, "[APU] readExternal(0x", hex(address, 6L), ")");
    break;
  }

  MegaDrive::bus.release(MegaDrive::Bus::APU);
  return data;
}

auto APU::writeExternal(n24 address, n8 data) -> void {
  //bus arbiter delay rough approximation
  step(3);
  while(MegaDrive::bus.acquired() && !scheduler.synchronizing()) step(1);
  MegaDrive::bus.acquire(MegaDrive::Bus::APU);

  switch(address) {
  case 0x000000 ... 0x9fffff:
  case 0xa10000 ... 0xa1ffff:
  case 0xc00000 ... 0xc000ff:
  case 0xe00000 ... 0xffffff:
    if(address & 1) {
      MegaDrive::bus.write(0, 1, address & ~1, data << 8 | data << 0);
    } else {
      MegaDrive::bus.write(1, 0, address & ~1, data << 8 | data << 0);
    }
    break;
  default:
    debug(unusual, "[APU] writeExternal(0x", hex(address, 6L), ")");
    break;
  }

  MegaDrive::bus.release(MegaDrive::Bus::APU);
}

auto APU::in(n16 address) -> n8 {
  //unused on Mega Drive
  return 0x00;
}

auto APU::out(n16 address, n8 data) -> void {
  //unused on Mega Drive
}
