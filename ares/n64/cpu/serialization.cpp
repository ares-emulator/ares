auto CPU::serialize(serializer& s) -> void {
  Thread::serialize(s);
  serializeR4300(s);
}
