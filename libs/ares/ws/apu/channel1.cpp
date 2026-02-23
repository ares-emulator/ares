auto APU::Channel1::tick() -> void {
  if(io.enable) {
    if(--state.period == io.pitch) {
      state.period = 0;
      state.sampleOffset++;
    }
  }
}

auto APU::Channel1::output() -> void {
  if(apu.sequencerHeld()) return;
  auto sample = apu.sample(1, state.sampleOffset);
  apu.io.output.left  += sample * io.volumeLeft;
  apu.io.output.right += sample * io.volumeRight;
}

auto APU::Channel1::power() -> void {
  io = {};
  state = {};
}
