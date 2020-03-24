auto PCD::serialize(serializer& s) -> void {
  Thread::serialize(s);
  wram.serialize(s);
  bram.serialize(s);
}
