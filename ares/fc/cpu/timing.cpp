auto CPU::read(n16 address) -> n8 {
  if(io.oamDMAPending || io.dmcDMAPending) {
    dma(address);
  }

  io.openBus = readBus(address);
  step(rate());
  return io.openBus;
}

auto CPU::write(n16 address, n8 data) -> void {
  writeBus(address, data);
  step(rate());
}

auto CPU::lastCycle() -> void {
  io.interruptPending = ((io.irqLine | io.apuLine) & !P.i) | io.nmiPending;
}

auto CPU::cancelNmi() -> void {
  io.interruptPending = ((io.irqLine | io.apuLine) & !P.i);
}

auto CPU::delayIrq() -> void {
  io.interruptPending = io.nmiPending;
}

auto CPU::irqPending() -> bool {
  return (io.irqLine | io.apuLine) & !P.i;
}

auto CPU::nmi(n16& vector) -> void {
  if(io.nmiPending) {
    io.nmiPending = false;
    vector = 0xfffa;
  }
}

auto CPU::dmcDMAPending() -> void {
  io.dmcDMAPending = 1;
  io.dmcDummyRead = 0;
}

auto CPU::dma(n16 address) -> void {
  bool oamWriteReady = false;
  n8   oamCounter = 0;
  bool skipDummyReads = (address == 0x4016 || address == 0x4017);

  // halt read
  io.openBus = readBus(address);
  step(rate());

  while (io.dmcDMAPending || io.oamDMAPending) {
    if (io.oddCycle) {
      // put_cycle
      if (io.oamDMAPending && oamWriteReady) {
        // oam write
        writeBus(0x2004, io.openBus);
        step(rate());

        oamWriteReady = false;
        if (++oamCounter == 0)
          io.oamDMAPending = 0;
      } else {
        // oam (re)alignment cycle
        // dmc alignment cycle
        io.dmcDummyRead = 1;

        if (!skipDummyReads)
          io.openBus = readBus(address);
        step(rate());
      }
    } else {
      // get_cycle
      if (io.dmcDMAPending && io.dmcDummyRead) {
        // dmc read
        io.openBus = readBus(apu.dmc.dmaAddress());
        step(rate());

        apu.dmc.setDMABuffer(io.openBus);
        io.dmcDMAPending = 0;
      } else if (io.oamDMAPending) {
        if (io.dmcDMAPending)
          io.dmcDummyRead = 1;

        // oam read
        io.openBus = readBus(io.oamDMAPage << 8 | oamCounter);
        step(rate());

        oamWriteReady = true;
      } else {
        io.dmcDummyRead = 1;

        // dmc dummy read cycle
        if (!skipDummyReads)
          io.openBus = readBus(address);
        step(rate());
      }
    }
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
