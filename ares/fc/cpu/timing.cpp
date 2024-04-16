auto CPU::read(n16 address) -> n8 {
  if(io.oamDMAPending) {
    io.oamDMAPending = 0;
    read(address);
    oamDMA();
  }

  while(io.rdyLine == 0) {
    io.openBus = readBus(io.rdyAddressValid ? io.rdyAddressValue : address);
    step(rate());
  }

  io.openBus = readBus(address);
  step(rate());
  return io.openBus;
}

auto CPU::write(n16 address, n8 data) -> void {
  writeBus(address, io.openBus = data);
  step(rate());
}

auto CPU::oamDMA() -> void {
  for(u32 n : range(256)) {
    n8 data = read(io.oamDMAPage << 8 | n);
    write(0x2004, data);
  }
}

auto CPU::irqLine(bool line) -> void {
  //level-sensitive
  io.irqLine = line;
  Ricoh2A03::irqLine(io.irqLine | io.apuLine);
}

auto CPU::apuLine(bool line) -> void {
  //level-sensitive
  io.apuLine = line;
  Ricoh2A03::irqLine(io.irqLine | io.apuLine);
}

auto CPU::rdyLine(bool line) -> void {
  io.rdyLine = line;
}

auto CPU::rdyAddress(bool valid, n16 value) -> void {
  io.rdyAddressValid = valid;
  io.rdyAddressValue = value;
}
