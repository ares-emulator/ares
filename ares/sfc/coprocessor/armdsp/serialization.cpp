auto ARMDSP::firmware() const -> vector<n8> {
  vector<n8> buffer;
  if(!cartridge.has.ARMDSP) return buffer;
  buffer.reserve(128_KiB + 32_KiB);
  for(u32 n : range(128_KiB)) buffer.append(programROM[n]);
  for(u32 n : range( 32_KiB)) buffer.append(dataROM[n]);
  return buffer;
}

auto ARMDSP::serialize(serializer& s) -> void {
  ARM7TDMI::serialize(s);
  Thread::serialize(s);
  s(array_span<u8>{programRAM, 16_KiB});
  s(bridge.cputoarm.ready);
  s(bridge.cputoarm.data);
  s(bridge.armtocpu.ready);
  s(bridge.armtocpu.data);
  s(bridge.timer);
  s(bridge.timerlatch);
  s(bridge.reset);
  s(bridge.ready);
  s(bridge.signal);
}
