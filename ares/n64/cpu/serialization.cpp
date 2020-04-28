auto CPU::serialize(serializer& s) -> void {
  VR4300::serialize(s);
  Thread::serialize(s);
}
