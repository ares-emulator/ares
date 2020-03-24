auto OPLL::serialize(serializer& s) -> void {
  YM2413::serialize(s);
  Thread::serialize(s);
}
