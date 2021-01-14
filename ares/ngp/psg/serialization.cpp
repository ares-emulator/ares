auto PSG::serialize(serializer& s) -> void {
  T6W28::serialize(s);
  Thread::serialize(s);
  s(psg.enable);
  s(dac.left);
  s(dac.right);
}
