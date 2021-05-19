auto APU::read(n16 address) -> n8 {
  switch(address) {
  case 0x0000 ... 0x7fff: return cartridge.mrom[address];
  case 0x8000 ... 0xbfff: return cartridge.mrom[io.romBank[3] << 14 | n14(address)];
  case 0xc000 ... 0xdfff: return cartridge.mrom[io.romBank[2] << 13 | n13(address)];
  case 0xe000 ... 0xefff: return cartridge.mrom[io.romBank[1] << 12 | n12(address)];
  case 0xf000 ... 0xf7ff: return cartridge.mrom[io.romBank[0] << 11 | n11(address)];
  case 0xf800 ... 0xffff: return ram[address];
  }
  unreachable;
}

auto APU::write(n16 address, n8 data) -> void {
  switch(address) {
  case 0xf800 ... 0xffff: ram[address] = data; return;
  }
}

auto APU::in(n16 address) -> n8 {
  switch(n8(address)) {
  case 0x00: return communication.input;
  case 0x04: return opnb.read(0);
  case 0x05: return opnb.read(1);
  case 0x06: return opnb.read(2);
  case 0x07: return opnb.read(3);
  case 0x08: io.romBank[0] = address.byte(1); return 0;
  case 0x09: io.romBank[1] = address.byte(1); return 0;
  case 0x0a: io.romBank[2] = address.byte(1); return 0;
  case 0x0b: io.romBank[3] = address.byte(1); return 0;
  case 0x18: io.romBank[0] = address.byte(1); return 0;  //0x08 mirror
  }
  return 0;
}

auto APU::out(n16 address, n8 data) -> void {
  switch(n8(address)) {
  case 0x00: communication.input = 0; return;
  case 0x04: return opnb.write(0, data);
  case 0x05: return opnb.write(1, data);
  case 0x06: return opnb.write(2, data);
  case 0x07: return opnb.write(3, data);
  case 0x08: io.nmiEnable = 1; return;
  case 0x09: io.nmiEnable = 1; return;
  case 0x0a: io.nmiEnable = 1; return;
  case 0x0b: io.nmiEnable = 1; return;
  case 0x0c: communication.output = data; return;
  case 0x18: io.nmiEnable = 0; return;
  }
}
