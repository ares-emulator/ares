auto CPU::serialize(serializer& s) -> void {
  MOS6502::serialize(s);
  Thread::serialize(s);
  s(ram);
  s(io.interruptPending);
  s(io.nmiPending);
  s(io.nmiLine);
  s(io.irqLine);
  s(io.apuLine);
  s(io.oddCycle);
  s(io.dmcDMAPending);
  s(io.oamDMAPending);
  s(io.oamDMAPage);
  s(io.openBus);
}
