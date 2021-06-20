auto APU::Channel4::noiseSample() -> n4 {
  return state.noiseOutput ? 0xf : 0x0;
}

auto APU::Channel4::run() -> void {
  if(--state.period == io.pitch) {
    state.period = 0;

    auto sample = io.noise ? noiseSample() : apu.sample(4, state.sampleOffset++);
    output.left  = sample * io.volumeLeft;
    output.right = sample * io.volumeRight;

    if(io.noiseReset) {
      io.noiseReset = 0;
      state.noiseLFSR = 0;
      state.noiseOutput = 0;
    }

    if(io.noiseUpdate) {
      static constexpr s32 taps[8] = {14, 10, 13, 4, 8, 6, 9, 11};
      auto tap = taps[io.noiseMode];

      state.noiseOutput = (1 ^ (state.noiseLFSR >> 7) ^ (state.noiseLFSR >> tap)) & 1;
      state.noiseLFSR = state.noiseLFSR << 1 | state.noiseOutput;
    }
  }
}

auto APU::Channel4::power() -> void {
  io = {};
  state = {};
  output = {};
}
