auto PSG::serialize(serializer& s) -> void {
  SN76489::serialize(s);
  Thread::serialize(s);
  s(io.mute);
  s(io.enable);
}
