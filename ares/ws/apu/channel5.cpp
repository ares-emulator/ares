auto APU::Channel5::dmaWrite(n8 sample) -> void {
  if (io.mode == 3 || (io.mode != 1 &&  state.channel)) state.right = sample;
  if (io.mode == 3 || (io.mode != 2 && !state.channel)) state.left = sample;

  state.channel ^= 1;
}

auto APU::Channel5::scale(i8 sample) -> i16 {
  if (!io.volume) return (sample << 8);
  
  switch(io.scale) {
  case 0: return ((n8)sample << 8 - io.volume);
  case 1: return ((n8)sample << 8 - io.volume) - (0x10000 >> io.volume);
  case 2: return (sample << 8 - io.volume);
  case 3: return (sample << 8);
  }

  unreachable;
}

auto APU::Channel5::runOutput() -> void {
  static constexpr s32 divisors[8] = {1, 2, 3, 4, 5, 6, 8, 12};
  if (++state.clock < divisors[io.speed]) return;
  state.clock = 0;

  output.left = scale(state.left);
  output.right = scale(state.right);
}

auto APU::Channel5::power() -> void {
  io = {};
  state = {};
  output = {};
}
