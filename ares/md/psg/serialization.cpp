auto PSG::serialize(serializer& s) -> void {
  SN76489::serialize(s);
  Thread::serialize(s);

  s(io.debugVolumeOverride);
  s(io.debugVolumeChannel);
}
