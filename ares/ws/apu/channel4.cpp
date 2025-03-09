auto APU::Channel4::noiseSample() -> n4 {
  return state.noiseOutput ? 0xf : 0x0;
}

auto APU::Channel4::tick() -> void {
  if(io.enable) {
    if(--state.period == io.pitch) {
      state.period = 0;
      state.sampleOffset++;

      if(io.noise && io.noiseUpdate && !apu.io.seqDbgNoise) {
        static constexpr s32 taps[8] = {14, 10, 13, 4, 8, 6, 9, 11};
        auto tap = taps[io.noiseMode];

        state.noiseOutput = (1 ^ (state.noiseLFSR >> 7) ^ (state.noiseLFSR >> tap)) & 1;
        state.noiseLFSR = state.noiseLFSR << 1 | state.noiseOutput;
      }
    }
  }
}

auto APU::Channel4::output() -> void {
  if(apu.sequencerHeld()) return;
  auto sample = io.noise ? noiseSample() : apu.sample(4, state.sampleOffset);
  apu.io.output.left  += sample * io.volumeLeft;
  apu.io.output.right += sample * io.volumeRight;
}

auto APU::Channel4::power() -> void {
  io = {};
  state = {};
}
