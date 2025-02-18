//70224 clocks/frame
//  456 clocks/scanline
//  154 scanlines/frame

auto CPU::step() -> void {
  step(1);
}

auto CPU::step(u32 clocks) -> void {
  //determine which bit of DIV is used to check timer state
  n32 timerBit = 9;
  if(status.timerClock != 0) timerBit = status.timerClock * 2 + 1;

  for(auto n : range(clocks)) {
    if(!r.stop) status.div++;

    //tick timer on falling edge
    n1 timerLineNew = status.div.bit(timerBit) & status.timerEnable;
    if(timerLineNew == 0 && status.timerLine == 1) timerTick();

    if((n9 )status.div == 0) timer8192hz();  //serial
    if((n12)status.div == 0) timer1024hz();  //joypad

    status.timerLine = timerLineNew;

    Thread::step(1);
    Thread::synchronize();
  }

  if(Model::SuperGameBoy()) {
    system.information.clocksExecuted += clocks;
  }
}

auto CPU::timerTick() -> void {
  if(++status.tima == 0) {
    status.tima = status.tma;
    raise(Interrupt::Timer);
  }
}

auto CPU::timer8192hz() -> void {
  if(status.serialTransfer && status.serialClock) {
    status.serialData <<= 1;
    status.serialData |= 1;
    if(--status.serialBits == 0) {
      status.serialTransfer = 0;
      raise(Interrupt::Serial);
    }
  }
}

auto CPU::timer1024hz() -> void {
  joypPoll();
}

auto CPU::hblank() -> void {
  status.hblankPending = 1;
}

auto CPU::hblankTrigger() -> void {
  if(status.hdmaActive && ppu.status.ly < 144) {
    for(u32 loop : range(16)) {
      writeDMA(status.dmaTarget++, readDMA(status.dmaSource++, 0xff));
      if(loop & 1) step(1 << status.speedDouble);
    }
    if(status.dmaLength-- == 0) {
      status.hdmaActive = 0;
    }
  }
}
