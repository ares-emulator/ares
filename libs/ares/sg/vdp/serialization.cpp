auto VDP::serialize(serializer& s) -> void {
  TMS9918::serialize(s);
  Thread::serialize(s);
}
