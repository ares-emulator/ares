auto CPU::serialize(serializer& s) -> void {
  MOS6502::serialize(s);
  Thread::serialize(s);
  s(io.resetPending);
  s(io.rdyLine);
  s(io.openBus);
}
