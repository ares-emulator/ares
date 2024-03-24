auto APU::Channel5::dmaWrite(n8 sample) -> void {
  if (io.mode == 0) {
    write(sample);
    state.channel ^= 1;
  }
  if (io.mode == 1 || io.mode == 3) {
    state.channel = 0;
    write(sample);
  }
  if (io.mode == 2 || io.mode == 3) {
    state.channel = 1;
    write(sample);
  }
}

auto APU::Channel5::manualWrite(n8 sample) -> void {
  write(sample);
  state.channel ^= 1;
}

auto APU::Channel5::write(n8 sample) -> void {
  if (state.channel) {
    state.right = sample;
    state.rightChanged = 1;
  } else {
    state.left = sample;
    state.leftChanged = 1;
  }
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

  if (state.leftChanged) {
    output.left = scale(state.left);
    state.leftChanged = 0;
  }
  if (state.rightChanged) {
    output.right = scale(state.right);
    state.rightChanged = 0;
  }
}

auto APU::Channel5::power() -> void {
  io = {};
  state = {};
  output = {};
}
