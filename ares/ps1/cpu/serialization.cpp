auto CPU::serialize(serializer& s) -> void {
  Thread::serialize(s);
  serializeCore(s);
}
