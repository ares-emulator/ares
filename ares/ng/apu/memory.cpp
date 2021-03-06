auto APU::read(n16 address) -> n8 {
  switch(address) {
  case 0x0000 ... 0x7fff: return cartridge.mrom[address];
  case 0x8000 ... 0xbfff: return cartridge.mrom[rom.bankA << 14 | n14(address)];
  case 0xc000 ... 0xdfff: return cartridge.mrom[rom.bankB << 13 | n13(address)];
  case 0xe000 ... 0xefff: return cartridge.mrom[rom.bankC << 12 | n12(address)];
  case 0xf000 ... 0xf7ff: return cartridge.mrom[rom.bankD << 11 | n11(address)];
  case 0xf800 ... 0xffff: return ram[address & 0x7ff];
  }
  unreachable;
}

auto APU::write(n16 address, n8 data) -> void {
  switch(address) {
  case 0xf800 ... 0xffff: ram[address & 0x7ff] = data; return;
  }
}

auto APU::in(n16 address) -> n8 {
  switch(n8(address & 0xf)) {
  case 0x00: nmi.pending = false; return communication.input;
  case 0x04: return opnb.read(0);
  case 0x05: return opnb.read(1);
  case 0x06: return opnb.read(2);
  case 0x07: return opnb.read(3);
  case 0x08: rom.bankD = address.byte(1); return 0;
  case 0x09: rom.bankC = address.byte(1); return 0;
  case 0x0a: rom.bankB = address.byte(1); return 0;
  case 0x0b: rom.bankA = address.byte(1); return 0;
  case 0x0c: return 0;  //SDRD1
  }
  return 0;
}

auto APU::out(n16 address, n8 data) -> void {
  switch(n8(address & 0x1f)) {
  case 0x08 ... 0x0b: nmi.enable = 1; return;
  case 0x18:          nmi.enable = 0; return;
  }

  switch(address & 0xf) {
  case 0x00: communication.input = 0;     return;
  case 0x04: return opnb.write(0, data);
  case 0x05: return opnb.write(1, data);
  case 0x06: return opnb.write(2, data);
  case 0x07: return opnb.write(3, data);
  case 0x0c: communication.output = data; return;
  }
}
