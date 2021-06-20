auto APU::Channel5::run() -> void {
  i11 sample;
  switch(io.scale) {
  case 0: sample = (n8)state.data << 3 - io.volume; break;
  case 1: sample = (n8)state.data - 0x100 << 3 - io.volume; break;
  case 2: sample = (i8)state.data << 3 - io.volume; break;
  case 3: sample = (n8)state.data << 3; break;
  }

  output.left  = io.leftEnable  ? sample : (i11)0;
  output.right = io.rightEnable ? sample : (i11)0;
}

auto APU::Channel5::power() -> void {
  io = {};
  state = {};
  output = {};
}
