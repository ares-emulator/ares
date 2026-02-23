auto OPN2::serialize(serializer& s) -> void {
  YM2612::serialize(s);
  Thread::serialize(s);
}
