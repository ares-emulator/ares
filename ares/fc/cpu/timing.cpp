auto CPU::read(uint16 address) -> uint8 {
  if(io.oamDMAPending) {
    io.oamDMAPending = 0;
    read(address);
    oamDMA();
  }

  while(io.rdyLine == 0) {
    r.mdr = readBus(io.rdyAddressValid ? io.rdyAddressValue : address);
    step(rate());
  }

  r.mdr = readBus(address);
  step(rate());
  return r.mdr;
}

auto CPU::write(uint16 address, uint8 data) -> void {
  writeBus(address, r.mdr = data);
  step(rate());
}

auto CPU::lastCycle() -> void {
  io.interruptPending = ((io.irqLine | io.apuLine) & ~r.p.i) | io.nmiPending;
}

auto CPU::nmi(uint16& vector) -> void {
  if(io.nmiPending) {
    io.nmiPending = false;
    vector = 0xfffa;
  }
}

auto CPU::oamDMA() -> void {
  for(uint n : range(256)) {
    uint8 data = read(io.oamDMAPage << 8 | n);
    write(0x2004, data);
  }
}

auto CPU::nmiLine(bool line) -> void {
  //edge-sensitive (0->1)
  if(!io.nmiLine && line) io.nmiPending = true;
  io.nmiLine = line;
}

auto CPU::irqLine(bool line) -> void {
  //level-sensitive
  io.irqLine = line;
}

auto CPU::apuLine(bool line) -> void {
  //level-sensitive
  io.apuLine = line;
}

auto CPU::rdyLine(bool line) -> void {
  io.rdyLine = line;
}

auto CPU::rdyAddress(bool valid, uint16 value) -> void {
  io.rdyAddressValid = valid;
  io.rdyAddressValue = value;
}
