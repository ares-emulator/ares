auto CPU::serialize(serializer& s) -> void {
  MOS6502::serialize(s);
  Thread::serialize(s);
  s(ram);
  s(state.interruptPending);
  s(state.resetPending);
  s(state.irqLine);
  s(state.timer);
  s(state.release);
}
