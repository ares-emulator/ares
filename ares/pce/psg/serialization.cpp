auto PSG::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(io.channel);
  s(io.volumeLeft);
  s(io.volumeRight);
  s(io.lfoFrequency);
  s(io.lfoControl);
  s(io.lfoEnable);

  for(auto& c : channel) {
    s(c.io.waveFrequency);
    s(c.io.volume);
    s(c.io.direct);
    s(c.io.enable);
    s(c.io.volumeLeft);
    s(c.io.volumeRight);
    s(c.io.waveBuffer);
    s(c.io.noiseFrequency);
    s(c.io.noiseEnable);
    s(c.io.wavePeriod);
    s(c.io.waveSample);
    s(c.io.waveOffset);
    s(c.io.noisePeriod);
    s(c.io.noiseSample);
    s(c.io.output);
  }
}
