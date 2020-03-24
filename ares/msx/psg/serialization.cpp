auto PSG::serialize(serializer& s) -> void {
  AY38910::serialize(s);
  Thread::serialize(s);
}
