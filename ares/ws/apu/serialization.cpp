auto APU::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(dma);
  s(channel1);
  s(channel2);
  s(channel3);
  s(channel4);
  s(channel5);

  s(io.waveBase);
  s(io.speakerEnable);
  s(io.speakerShift);
  s(io.headphonesEnable);
  s(io.headphonesConnected);
  s(io.masterVolume);
  s(io.output.left);
  s(io.output.right);
  s(state.sweepClock);
  s(state.apuClock);
}

auto APU::DMA::serialize(serializer& s) -> void {
  s(io.source);
  s(io.length);
  s(io.rate);
  s(io.unknown);
  s(io.loop);
  s(io.target);
  s(io.direction);
  s(io.enable);
  s(state.clock);
  s(state.source);
  s(state.length);
}

auto APU::Channel1::serialize(serializer& s) -> void {
  s(io.pitch);
  s(io.volumeLeft);
  s(io.volumeRight);
  s(io.enable);
  s(state.period);
  s(state.sampleOffset);
}

auto APU::Channel2::serialize(serializer& s) -> void {
  s(io.pitch);
  s(io.volumeLeft);
  s(io.volumeRight);
  s(io.enable);
  s(io.voice);
  s(io.voiceEnableLeftHalf);
  s(io.voiceEnableLeftFull);
  s(io.voiceEnableRightHalf);
  s(io.voiceEnableRightFull);
  s(state.period);
  s(state.sampleOffset);
}

auto APU::Channel3::serialize(serializer& s) -> void {
  s(io.pitch);
  s(io.volumeLeft);
  s(io.volumeRight);
  s(io.sweepValue);
  s(io.sweepTime);
  s(io.enable);
  s(io.sweep);
  s(state.period);
  s(state.sampleOffset);
  s(state.sweepCounter);
}

auto APU::Channel4::serialize(serializer& s) -> void {
  s(io.pitch);
  s(io.volumeLeft);
  s(io.volumeRight);
  s(io.noiseMode);
  s(io.noiseReset);
  s(io.noiseUpdate);
  s(io.enable);
  s(io.noise);
  s(state.period);
  s(state.sampleOffset);
  s(state.noiseOutput);
  s(state.noiseLFSR);
}

auto APU::Channel5::serialize(serializer& s) -> void {
  s(io.volume);
  s(io.scale);
  s(io.speed);
  s(io.enable);
  s(io.unknown);
  s(io.leftEnable);
  s(io.rightEnable);
  s(state.clock);
  s(state.data);
  s(output.left);
  s(output.right);
}
