auto Cartridge::serialize(serializer& s) -> void {
  Thread::serialize(s);
  ram.serialize(s);
  rtc.serialize(s);

  s.integer(bootromEnable);

  if(mapper) mapper->serialize(s);
}
