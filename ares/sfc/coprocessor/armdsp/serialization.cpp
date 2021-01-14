auto ARMDSP::firmware() const -> vector<uint8> {
  vector<uint8> buffer;
  if(!cartridge.has.ARMDSP) return buffer;
  buffer.reserve(128 * 1024 + 32 * 1024);
  for(uint n : range(128 * 1024)) buffer.append(programROM[n]);
  for(uint n : range( 32 * 1024)) buffer.append(dataROM[n]);
  return buffer;
}

auto ARMDSP::serialize(serializer& s) -> void {
  ARM7TDMI::serialize(s);
  Thread::serialize(s);
  s(array_span<uint8_t>{programRAM, 16 * 1024});
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
