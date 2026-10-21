auto Timer::readByte(u32 address) -> u32 {
  n32 data;
  debug(unhandled, "Timer::readByte(", hex(address, 8L), ") -> ", hex(data, 2L));
  return data;
}

auto Timer::readHalf(u32 address) -> u32 {
  n32 data;
  debug(unhandled, "Timer::readHalf(", hex(address, 8L), ") -> ", hex(data, 4L));
  return data;
}

auto Timer::readWord(u32 address) -> u32 {
  n32 data;
  u32 c = address >> 4 & 3;
  u32 addr = address & 0x1fff'ffcf;

  if(addr == 0x1f80'1100 && c <= 2) {
    data.bit(0,15) = timers[c].counter;
    return data;
  }

  if(addr == 0x1f80'1100 && c == 3) {
    data.bit(0,15) = 0x0030;
    return data;
  }

  if(addr == 0x1f80'1104 && c <= 2) {
    data.bit( 0)    = timers[c].synchronize;
    data.bit( 1, 2) = timers[c].mode;
    data.bit( 3)    = timers[c].resetMode;
    data.bit( 4)    = timers[c].irqOnTarget;
    data.bit( 5)    = timers[c].irqOnSaturate;
    data.bit( 6)    = timers[c].irqRepeat;
    data.bit( 7)    = timers[c].irqMode;
    data.bit( 8)    = timers[c].clock;
    data.bit( 9)    = timers[c].divider;
    data.bit(10)    = timers[c].irqLine;
    data.bit(11)    = timers[c].reachedTarget;
    data.bit(12)    = timers[c].reachedSaturate;
    timers[c].reachedTarget   = 0;
    timers[c].reachedSaturate = 0;
    return data;
  }

  if(addr == 0x1f80'1104 && c == 3) return data;

  if(addr == 0x1f80'1108 && c <= 2) {
    data.bit(0,15) = timers[c].target;
    return data;
  }

  if(addr == 0x1f80'1108 && c == 3) return data;

  debug(unhandled, "Timer::readWord(", hex(address, 8L), ") -> ", hex(data, 8L));
  return data;
}

auto Timer::writeByte(u32 address, u32 data) -> void {
  debug(unhandled, "Timer::writeByte(", hex(address, 8L), ", ", hex(data, 2L), ")");
}

auto Timer::writeHalf(u32 address, u32 data) -> void {
  debug(unhandled, "Timer::writeHalf(", hex(address, 8L), ", ", hex(data, 4L), ")");
}

auto Timer::writeWord(u32 address, u32 value) -> void {
  n32 data = value;
  u32 c = address >> 4 & 3;
  u32 addr = address & 0x1fff'ffcf;

  if(addr == 0x1f80'1100 && c <= 2) {
    timers[c].counter = data.bit(0,15);
    timers[c].resetPending = 0;
    timers[c].wait = WAIT_CYCLES;
    updateNextEvent();
    return;
  }

  if(addr == 0x1f80'1100 && c == 3) return;

  if(addr == 0x1f80'1104 && c <= 2) {
    timers[c].synchronize   = data.bit( 0);
    timers[c].syncFreeRun   = 0;
    timers[c].mode          = data.bit( 1, 2);
    timers[c].resetMode     = data.bit( 3);
    timers[c].irqOnTarget   = data.bit( 4);
    timers[c].irqOnSaturate = data.bit( 5);
    timers[c].irqRepeat     = data.bit( 6);
    timers[c].irqMode       = data.bit( 7);
    timers[c].clock         = data.bit( 8);
    timers[c].divider       = data.bit( 9);
    timers[c].irqLine       = 1;
    interrupt.lower(Interrupt::Timer0 + c);

    if(c == 0) {
      if(timers[0].synchronize && timers[0].mode == 0) timers[0].paused = gpu.hblank();
      if(timers[0].synchronize && timers[0].mode == 1) timers[0].paused = 0;
      if(timers[0].synchronize && timers[0].mode == 2) timers[0].paused = !gpu.hblank();
      if(timers[0].synchronize && timers[0].mode == 3) timers[0].paused = 1;
      if(!timers[0].synchronize) timers[0].paused = 0;
    }

    if(c == 1) {
      if(timers[1].synchronize && timers[1].mode == 0) timers[1].paused = gpu.vblank();
      if(timers[1].synchronize && timers[1].mode == 1) timers[1].paused = 0;
      if(timers[1].synchronize && timers[1].mode == 2) timers[1].paused = !gpu.vblank();
      if(timers[1].synchronize && timers[1].mode == 3) timers[1].paused = 1;
      if(!timers[1].synchronize) timers[1].paused = 0;
    }

    if(c == 2) timers[2].paused = timers[2].mode == 3 || timers[2].mode == 0;

    timers[c].counter      = 0;
    timers[c].irqTriggered = 0;
    timers[c].resetPending = 0;
    timers[c].wait = WAIT_CYCLES;
    updateNextEvent();
    return;
  }

  if(addr == 0x1f80'1104 && c == 3) return;

  if(addr == 0x1f80'1108 && c <= 2) {
    timers[c].target = data.bit(0,15);
    updateNextEvent();
    return;
  }

  if(addr == 0x1f80'1108 && c == 3) return;

  debug(unhandled, "Timer::writeWord(", hex(address, 8L), ", ", hex(data, 8L), ") -> ");
}
