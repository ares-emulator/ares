auto APU::Channel2::run() -> void {
  if(io.voice) {
    n8 volume = io.volumeLeft << 4 | io.volumeRight << 0;
    output.left  = io.voiceEnableLeft  ? volume : (n8)0;
    output.right = io.voiceEnableRight ? volume : (n8)0;
  } else if(--state.period == io.pitch) {
    state.period = 0;
    auto sample = apu.sample(2, state.sampleOffset++);
    output.left  = sample * io.volumeLeft;
    output.right = sample * io.volumeRight;
  }
}

auto APU::Channel2::power() -> void {
  io = {};
  state = {};
  output = {};
}
