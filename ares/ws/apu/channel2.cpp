auto APU::Channel2::run() -> void {
  if (!io.voice) {
    if(--state.period == io.pitch) {
      state.period = 0;
      state.sampleOffset++;
    }
  }
}

auto APU::Channel2::runOutput() -> void {
  if(io.voice) {
    n8 volume = io.volumeLeft << 4 | io.volumeRight << 0;
    output.left  = io.voiceEnableLeftFull  ? volume : (n8)(io.voiceEnableLeftHalf  ? (volume >> 1) : 0);
    output.right = io.voiceEnableRightFull ? volume : (n8)(io.voiceEnableRightHalf ? (volume >> 1) : 0);
  } else {
    auto sample = apu.sample(2, state.sampleOffset);
    output.left  = sample * io.volumeLeft;
    output.right = sample * io.volumeRight;
  }
}

auto APU::Channel2::power() -> void {
  io = {};
  state = {};
  output = {};
}
