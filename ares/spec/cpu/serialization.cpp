auto CPU::serialize(serializer& s) -> void {
  Z80::serialize(s);
  Thread::serialize(s);

  ram.serialize(s);
}
