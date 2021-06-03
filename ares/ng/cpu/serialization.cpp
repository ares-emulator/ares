auto CPU::serialize(serializer& s) -> void {
  M68000::serialize(s);
  Thread::serialize(s);
}
