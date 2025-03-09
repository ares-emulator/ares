auto APU::Channel3::sweep() -> void {
  if(io.sweep && --state.sweepCounter < 0) {
    state.sweepCounter = io.sweepTime;
    io.pitch += io.sweepValue;
  }
}

auto APU::Channel3::tick() -> void {
  if(io.enable) {
    if(--state.period == io.pitch) {
      state.period = 0;
      state.sampleOffset++;
    }
  }
}

auto APU::Channel3::output() -> void {
  if(apu.sequencerHeld()) return;
  auto sample = apu.sample(3, state.sampleOffset);
  apu.io.output.left  += sample * io.volumeLeft;
  apu.io.output.right += sample * io.volumeRight;
}

auto APU::Channel3::power() -> void {
  io = {};
  state = {};
}
