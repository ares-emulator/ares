auto APU::Channel2::tick() -> void {
  if(!io.voice) {
    if(--state.period == io.pitch) {
      state.period = 0;
      state.sampleOffset++;
    }
  }
}

auto APU::Channel2::output() -> void {
  if(apu.sequencerHeld()) return;
  if(io.voice) {
    n8 volume = io.volumeLeft << 4 | io.volumeRight << 0;
    apu.io.output.left  += io.voiceEnableLeftFull  ? volume : (n8)(io.voiceEnableLeftHalf  ? (volume >> 1) : 0);
    apu.io.output.right += io.voiceEnableRightFull ? volume : (n8)(io.voiceEnableRightHalf ? (volume >> 1) : 0);
  } else {
    auto sample = apu.sample(2, state.sampleOffset);
    apu.io.output.left  += sample * io.volumeLeft;
    apu.io.output.right += sample * io.volumeRight;
  }
}

auto APU::Channel2::power() -> void {
  io = {};
  state = {};
}
