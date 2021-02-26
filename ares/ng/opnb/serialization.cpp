auto OPNB::serialize(serializer& s) -> void {
  YM2610::serialize(s);
  Thread::serialize(s);
}
