auto APU::DMA::run() -> void {
  if(!io.enable) return;

  if(io.rate == 0 && ++state.clock < 6) return;  // 4000hz
  if(io.rate == 1 && ++state.clock < 4) return;  // 6000hz
  if(io.rate == 2 && ++state.clock < 2) return;  //12000hz
  //24000hz runs always
  state.clock = 0;

  n8 data = 0x00;
  if(!io.hold) {
    data = bus.read(state.source);
    if(io.direction == 0) state.source++;
    if(io.direction == 1) state.source--;
  }

  if(io.target == 0) {
    apu.channel2.io.volumeRight = data.bit(0,3);
    apu.channel2.io.volumeLeft  = data.bit(4,7);
  } else {
    apu.channel5.state.data = data;
  }

  if(io.hold) return;
  if(--state.length) return;

  if(io.loop) {
    state.source = io.source;
    state.length = io.length;
  } else {
    io.enable = 0;
  }
}

auto APU::DMA::power() -> void {
  io = {};
  state = {};
}
