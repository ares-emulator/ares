auto PSG::serialize(serializer& s) -> void {
  T6W28::serialize(s);
  Thread::serialize(s);

  s.integer(psg.enable);
  s.integer(dac.left);
  s.integer(dac.right);
}
