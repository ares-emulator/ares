auto CPU::serialize(serializer& s) -> void {
  MOS6502::serialize(s);
  Thread::serialize(s);

  s(io.interruptPending);
  s(io.resetPending);
  s(io.nmiPending);
  s(io.nmiLine);
  s(io.irqLine);
  s(io.rdyLine);
  s(io.openBus);
}
