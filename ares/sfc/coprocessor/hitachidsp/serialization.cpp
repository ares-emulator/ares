auto HitachiDSP::firmware() const -> std::vector<n8> {
  std::vector<n8> buffer;
  if(!cartridge.has.HitachiDSP) return buffer;
  buffer.reserve(3_KiB);
  for(u32 n : range(1024)) {
    buffer.push_back(dataROM[n] >>  0);
    buffer.push_back(dataROM[n] >>  8);
    buffer.push_back(dataROM[n] >> 16);
  }
  return buffer;
}

auto HitachiDSP::serialize(serializer& s) -> void {
  HG51B::serialize(s);
  Thread::serialize(s);
}
