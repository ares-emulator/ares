auto CPU::serialize(serializer& s) -> void {
  MOS6502::serialize(s);
  Thread::serialize(s);

  ram.serialize(s);

  s.integer(io.interruptPending);
  s.integer(io.nmiPending);
  s.integer(io.nmiLine);
  s.integer(io.irqLine);
  s.integer(io.apuLine);

  s.integer(io.rdyLine);
  s.integer(io.rdyAddressValid);
  s.integer(io.rdyAddressValue);

  s.integer(io.oamDMAPending);
  s.integer(io.oamDMAPage);
}
