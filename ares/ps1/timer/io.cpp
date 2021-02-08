auto Timer::readByte(u32 address) -> u32 {
  return readWord(address & ~3) >> 8 * (address & 3);
}

auto Timer::readHalf(u32 address) -> u32 {
  return readWord(address & ~3) >> 8 * (address & 3);
}

auto Timer::readWord(u32 address) -> u32 {
  n32 data;
  u32 c = address >> 4 & 3;

  if((address & 0xffff'ffcf) == 0x1f80'1100 && c <= 2) {
    data.bit(0,15) = timers[c].counter;
  }

  if((address & 0xffff'ffcf) == 0x1f80'1104 && c <= 2) {
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
    data.bit(13,15) = timers[c].unknown;
    timers[c].reachedTarget   = 0;
    timers[c].reachedSaturate = 0;
  }

  if((address & 0xffff'ffcf) == 0x1f80'1108 && c <= 2) {
    data.bit(0,15) = timers[c].target;
  }

  return data;
}

auto Timer::writeByte(u32 address, u32 data) -> void {
  return writeWord(address & ~3, data << 8 * (address & 3));
}

auto Timer::writeHalf(u32 address, u32 data) -> void {
  return writeWord(address & ~3, data << 8 * (address & 3));
}

auto Timer::writeWord(u32 address, u32 value) -> void {
  n32 data = value;
  u32 c = address >> 4 & 3;

  if((address & 0xffff'ffcf) == 0x1f80'1100 && c <= 2) {
    timers[c].reset();
  }

  if((address & 0xffff'ffcf) == 0x1f80'1104 && c <= 2) {
    timers[c].synchronize   = data.bit( 0);
    timers[c].mode          = data.bit( 1, 2);
    timers[c].resetMode     = data.bit( 3);
    timers[c].irqOnTarget   = data.bit( 4);
    timers[c].irqOnSaturate = data.bit( 5);
    timers[c].irqRepeat     = data.bit( 6);
    timers[c].irqMode       = data.bit( 7);
    timers[c].clock         = data.bit( 8);
    timers[c].divider       = data.bit( 9);
    timers[c].irqLine       = 1;
    timers[c].unknown       = data.bit(13,15);

    if(c == 0) timers[0].paused = timers[0].mode == 3;
    if(c == 1) timers[1].paused = timers[1].mode == 3;
    if(c == 2) timers[2].paused = timers[2].mode == 3 || timers[2].mode == 0;

    timers[c].counter      = 0;
    timers[c].irqTriggered = 0;
  }

  if((address & 0xffff'ffcf) == 0x1f80'1108 && c <= 2) {
    timers[c].target = data.bit(0,15);
  }
}
