auto Timer::readByte(u32 address) -> u32 {
  print("* rb", hex(address, 8L), "\n");
  return 0;
}

auto Timer::readHalf(u32 address) -> u32 {
  uint16 data;
  uint c = address >> 4 & 3;

  if((address & 0xffff'ffcf) == 0x1f80'1100 && c <= 2) {
    data.bit(0,15) = timers[c].counter;
  }

  if((address & 0xffff'ffcf) == 0x1f80'1104 && c <= 2) {
    data.bit( 0)    = timers[c].sync;
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

auto Timer::readWord(u32 address) -> u32 {
  uint32 data = readHalf(address & ~3 | 0) <<  0;
  return data | readHalf(address & ~3 | 2) << 16;
}

auto Timer::writeByte(u32 address, u32 value) -> void {
  print("* wb", hex(address, 8L), "\n");
}

auto Timer::writeHalf(u32 address, u32 value) -> void {
  uint16 data = value;
  uint c = address >> 4 & 3;

  if((address & 0xffff'ffcf) == 0x1f80'1100 && c <= 2) {
    timers[c].reset();
  }

  if((address & 0xffff'ffcf) == 0x1f80'1104 && c <= 2) {
    timers[c].sync            = data.bit( 0);
    timers[c].mode            = data.bit( 1, 2);
    timers[c].resetMode       = data.bit( 3);
    timers[c].irqOnTarget     = data.bit( 4);
    timers[c].irqOnSaturate   = data.bit( 5);
    timers[c].irqRepeat       = data.bit( 6);
    timers[c].irqMode         = data.bit( 7);
    timers[c].clock           = data.bit( 8);
    timers[c].divider         = data.bit( 9);
    timers[c].irqLine         = 1;
    timers[c].reachedTarget   = data.bit(11);
    timers[c].reachedSaturate = data.bit(12);
    timers[c].unknown         = data.bit(13,15);
    poll();
  }

  if((address & 0xffff'ffcf) == 0x1f80'1108 && c <= 2) {
    timers[c].target = data.bit(0,15);
  }
}

auto Timer::writeWord(u32 address, u32 data) -> void {
  writeHalf(address & ~3 | 0, data >>  0);
  writeHalf(address & ~3 | 2, data >> 16);
}
