auto APU::read(n16 address) -> n8 {
  if(address >= 0x0000 && address <= 0x0fff) {
  //u32 timeout = 0;
  //while(cpu.mar >= 0x7000 && cpu.mar <= 0x7fff && ++timeout < 64) step(1);
    return ram.read(0x3000 | address);
  }
  if(address == 0x8000) return port.data;
  return 0x00;
}

auto APU::write(n16 address, n8 data) -> void {
  if(address >= 0x0000 && address <= 0x0fff) {
  //u32 timeout = 0;
  //while(cpu.mar >= 0x7000 && cpu.mar <= 0x7fff && ++timeout < 64) step(1);
    return ram.write(0x3000 | address, data);
  }
  if(address == 0x4000) return psg.writeRight(data);
  if(address == 0x4001) return psg.writeLeft(data);
  if(address == 0x8000) return (void)(port.data = data);
  if(address == 0xc000) return cpu.int5.raise();
}

auto APU::in(n16 address) -> n8 {
  return 0x00;
}

auto APU::out(n16 address, n8 data) -> void {
  switch((n8)address) {
  case 0xff:
    irq.line = 0;
    cpu.int5.lower();
    break;
  }
}
