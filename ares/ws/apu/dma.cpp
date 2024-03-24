auto APU::DMA::run() -> void {
  if(!io.enable) return;

  if(io.rate == 0 && ++state.clock < 6) return;  // 4000hz
  if(io.rate == 1 && ++state.clock < 4) return;  // 6000hz
  if(io.rate == 2 && ++state.clock < 2) return;  //12000hz
  //24000hz runs always
  state.clock = 0;

  n8 data = 0x00;
  if(!io.hold) {
    data = bus.read(io.source);
    if(io.direction == 0) io.source++;
    if(io.direction == 1) io.source--;
  }

  if(io.target == 0) {
    apu.channel2.io.volumeRight = data.bit(0,3);
    apu.channel2.io.volumeLeft  = data.bit(4,7);
  } else {
    apu.channel5.dmaWrite(data);
  }

  if(io.hold) return;
  if(--io.length) return;

  if(io.loop) {
    io.source = state.source;
    io.length = state.length;
  } else {
    io.enable = 0;
  }
}

auto APU::DMA::power() -> void {
  io = {};
  state = {};
}
