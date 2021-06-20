auto APU::Channel3::sweep() -> void {
  if(io.sweep && --state.sweepCounter < 0) {
    state.sweepCounter = io.sweepTime;
    io.pitch += io.sweepValue;
  }
}

auto APU::Channel3::run() -> void {
  if(--state.period == io.pitch) {
    state.period = 0;
    auto sample = apu.sample(3, state.sampleOffset++);
    output.left  = sample * io.volumeLeft;
    output.right = sample * io.volumeRight;
  }
}

auto APU::Channel3::power() -> void {
  io = {};
  state = {};
  output = {};
}
