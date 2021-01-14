auto CPU::serialize(serializer& s) -> void {
  MOS6502::serialize(s);
  Thread::serialize(s);
  s(ram);
  s(io.interruptPending);
  s(io.nmiPending);
  s(io.nmiLine);
  s(io.irqLine);
  s(io.apuLine);
  s(io.rdyLine);
  s(io.rdyAddressValid);
  s(io.rdyAddressValue);
  s(io.oamDMAPending);
  s(io.oamDMAPage);
}
