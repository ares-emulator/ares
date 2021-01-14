auto APU::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(this->s.sweepClock);
  s(r.waveBase);
  s(r.speakerEnable);
  s(r.speakerShift);
  s(r.headphonesEnable);
  s(r.headphonesConnected);
  s(r.masterVolume);

  s(dma.s.clock);
  s(dma.s.source);
  s(dma.s.length);
  s(dma.r.source);
  s(dma.r.length);
  s(dma.r.rate);
  s(dma.r.unknown);
  s(dma.r.loop);
  s(dma.r.target);
  s(dma.r.direction);
  s(dma.r.enable);

  s(channel1.o.left);
  s(channel1.o.right);
  s(channel1.s.period);
  s(channel1.s.sampleOffset);
  s(channel1.r.pitch);
  s(channel1.r.volumeLeft);
  s(channel1.r.volumeRight);
  s(channel1.r.enable);

  s(channel2.o.left);
  s(channel2.o.right);
  s(channel2.s.period);
  s(channel2.s.sampleOffset);
  s(channel2.r.pitch);
  s(channel2.r.volumeLeft);
  s(channel2.r.volumeRight);
  s(channel2.r.enable);
  s(channel2.r.voice);
  s(channel2.r.voiceEnableLeft);
  s(channel2.r.voiceEnableRight);

  s(channel3.o.left);
  s(channel3.o.right);
  s(channel3.s.period);
  s(channel3.s.sampleOffset);
  s(channel3.s.sweepCounter);
  s(channel3.r.pitch);
  s(channel3.r.volumeLeft);
  s(channel3.r.volumeRight);
  s(channel3.r.sweepValue);
  s(channel3.r.sweepTime);
  s(channel3.r.enable);
  s(channel3.r.sweep);

  s(channel4.o.left);
  s(channel4.o.right);
  s(channel4.s.period);
  s(channel4.s.sampleOffset);
  s(channel4.s.noiseOutput);
  s(channel4.s.noiseLFSR);
  s(channel4.r.pitch);
  s(channel4.r.volumeLeft);
  s(channel4.r.volumeRight);
  s(channel4.r.noiseMode);
  s(channel4.r.noiseReset);
  s(channel4.r.noiseUpdate);
  s(channel4.r.enable);
  s(channel4.r.noise);

  s(channel5.o.left);
  s(channel5.o.right);
  s(channel5.s.clock);
  s(channel5.s.data);
  s(channel5.r.volume);
  s(channel5.r.scale);
  s(channel5.r.speed);
  s(channel5.r.enable);
  s(channel5.r.unknown);
  s(channel5.r.leftEnable);
  s(channel5.r.rightEnable);
}
