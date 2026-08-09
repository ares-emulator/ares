auto CPU::read(n16 address) -> n8 {
  while(!io.rdyLine) step(1);
  auto data = readBus(address);
  io.openBus = data;
  step(1);
  return data;
}

auto CPU::write(n16 address, n8 data) -> void {
  writeBus(address, data);
  io.openBus = data;
  step(1);
}

auto CPU::lastCycle() -> void {
  io.interruptPending = io.nmiPending | irqPending();
}

auto CPU::cancelNmi() -> void {
  io.interruptPending = irqPending();
}

auto CPU::delayIrq() -> void {
  io.interruptPending = io.nmiPending;
}

auto CPU::irqPending() -> bool {
  return io.irqLine && !P.i;
}

auto CPU::nmi(n16& vector) -> void {
  if(io.nmiPending) {
    io.nmiPending = 0;
    vector = 0xfffa;
  }
}

auto CPU::nmiLine(bool line) -> void {
  if(!io.nmiLine && line) io.nmiPending = 1;
  io.nmiLine = line;
}

auto CPU::irqLine(bool line) -> void {
  io.irqLine = line;
}

auto CPU::rdyLine(bool line) -> void {
  io.rdyLine = line;
}
