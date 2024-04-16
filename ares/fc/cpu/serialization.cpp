auto CPU::serialize(serializer& s) -> void {
  Ricoh2A03::serialize(s);
  Thread::serialize(s);
  s(ram);
  s(io.irqLine);
  s(io.apuLine);
  s(io.rdyLine);
  s(io.rdyAddressValid);
  s(io.rdyAddressValue);
  s(io.oamDMAPending);
  s(io.oamDMAPage);
  s(io.openBus);
}
