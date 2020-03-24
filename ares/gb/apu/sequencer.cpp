auto APU::Sequencer::run() -> void {
  if(enable == false) {
    center = 0;
    left   = 0;
    right  = 0;
    return;
  }

  int sample = 0;
  sample += apu.square1.output;
  sample += apu.square2.output;
  sample +=    apu.wave.output;
  sample +=   apu.noise.output;
  center = (sample * 512) - 16384;

  sample = 0;
  if(square1.leftEnable) sample += apu.square1.output;
  if(square2.leftEnable) sample += apu.square2.output;
  if(   wave.leftEnable) sample +=    apu.wave.output;
  if(  noise.leftEnable) sample +=   apu.noise.output;
  sample = (sample * 512) - 16384;
  sample = (sample * (leftVolume + 1)) / 8;
  left = sample;

  sample = 0;
  if(square1.rightEnable) sample += apu.square1.output;
  if(square2.rightEnable) sample += apu.square2.output;
  if(   wave.rightEnable) sample +=    apu.wave.output;
  if(  noise.rightEnable) sample +=   apu.noise.output;
  sample = (sample * 512) - 16384;
  sample = (sample * (rightVolume + 1)) / 8;
  right = sample;

  //reduce audio volume
  center >>= 1;
  left   >>= 1;
  right  >>= 1;
}

auto APU::Sequencer::power() -> void {
  leftEnable = 0;
  leftVolume = 0;
  rightEnable = 0;
  rightVolume = 0;
  noise.leftEnable = 0;
  wave.leftEnable = 0;
  square2.leftEnable = 0;
  square1.leftEnable = 0;
  noise.rightEnable = 0;
  wave.rightEnable = 0;
  square2.rightEnable = 0;
  square1.rightEnable = 0;
  enable = 0;

  center = 0;
  left   = 0;
  right  = 0;
}

auto APU::Sequencer::serialize(serializer& s) -> void {
  s.integer(leftEnable);
  s.integer(leftVolume);
  s.integer(rightEnable);
  s.integer(rightVolume);
  s.integer(noise.leftEnable);
  s.integer(wave.leftEnable);
  s.integer(square2.leftEnable);
  s.integer(square1.leftEnable);
  s.integer(noise.rightEnable);
  s.integer(wave.rightEnable);
  s.integer(square2.rightEnable);
  s.integer(square1.rightEnable);
  s.integer(enable);

  s.integer(center);
  s.integer(left);
  s.integer(right);
}
