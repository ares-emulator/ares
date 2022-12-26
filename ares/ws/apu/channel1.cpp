auto APU::Channel1::run() -> void {
  if(--state.period == io.pitch) {
    state.period = 0;
    state.sampleOffset++;
  }
}

auto APU::Channel1::runOutput() -> void {
  auto sample = apu.sample(1, state.sampleOffset);
  output.left  = sample * io.volumeLeft;
  output.right = sample * io.volumeRight;
}

auto APU::Channel1::power() -> void {
  io = {};
  state = {};
  output = {};
}
