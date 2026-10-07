auto CPU::serialize(serializer& s) -> void {
  I8080::serialize(s);
  Thread::serialize(s);
  s(ram);
  s(state.irqLine);
}
