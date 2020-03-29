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

  s.array(programRAM, 16 * 1024);

  s.integer(bridge.cputoarm.ready);
  s.integer(bridge.cputoarm.data);
  s.integer(bridge.armtocpu.ready);
  s.integer(bridge.armtocpu.data);
  s.integer(bridge.timer);
  s.integer(bridge.timerlatch);
  s.integer(bridge.reset);
  s.integer(bridge.ready);
  s.integer(bridge.signal);
}
