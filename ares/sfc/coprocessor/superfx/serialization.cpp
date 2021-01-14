auto SuperFX::serialize(serializer& s) -> void {
  GSU::serialize(s);
  Thread::serialize(s);
  s(ram);
  s(bram);
}
