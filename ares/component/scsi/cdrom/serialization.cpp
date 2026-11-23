auto SCSICDROM::serialize(serializer& s) -> void {
  s(status);
  s(position);
  s(readLBA);
  s(readRemaining);
  s(raw);
  s(sectorOffset);
  s(sectorLength);
  s(sectorPosition);
  s(blockSize);
  s(audioStatus);
  s(audioPosition);
  s(audioEnd);
  s(senseKey);
  s(senseASC);
  s(senseASCQ);
  s(outRemaining);
  s(outModeSelect);
  u32 size = buffer.size();
  s(size);
  if(s.reading()) buffer.resize(size);
  for(auto& byte : buffer) s(byte);
  u32 outSize = outBuffer.size();
  s(outSize);
  if(s.reading()) outBuffer.resize(outSize);
  for(auto& byte : outBuffer) s(byte);
}
