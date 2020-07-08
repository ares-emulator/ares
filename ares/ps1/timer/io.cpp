auto Timer::readByte(u32 address) -> u32 {
  print("* rb", hex(address, 8L), "\n");
  return 0;
}

auto Timer::readHalf(u32 address) -> u32 {
  uint16 data;

  if(address == 0x1f80'1100) {
    data.bit(0,15) = htimer.counter;
  }

  if(address == 0x1f80'1104) {
    data.bit( 0)    = htimer.synchronizeEnable;
    data.bit( 1, 2) = htimer.synchronizeMode;
    data.bit( 3)    = htimer.resetMode;
    data.bit( 4)    = htimer.irqOnTarget;
    data.bit( 5)    = htimer.irqOnSaturate;
    data.bit( 6)    = htimer.irqRepeat;
    data.bit( 7)    = htimer.irqMode;
    data.bit( 8, 9) = htimer.clockSource;
    data.bit(10)    = htimer.irqLine;
    data.bit(11)    = htimer.reachedTarget;
    data.bit(12)    = htimer.reachedSaturate;
    data.bit(13,15) = htimer.unknown;
    htimer.reachedTarget   = 0;
    htimer.reachedSaturate = 0;
  }

  if(address == 0x1f80'1108) {
    data.bit(0,15) = htimer.target;
  }

  if(address == 0x1f80'1110) {
    data.bit(0,15) = vtimer.counter;
  }

  if(address == 0x1f80'1114) {
    data.bit( 0)    = vtimer.synchronizeEnable;
    data.bit( 1, 2) = vtimer.synchronizeMode;
    data.bit( 3)    = vtimer.resetMode;
    data.bit( 4)    = vtimer.irqOnTarget;
    data.bit( 5)    = vtimer.irqOnSaturate;
    data.bit( 6)    = vtimer.irqRepeat;
    data.bit( 7)    = vtimer.irqMode;
    data.bit( 8, 9) = vtimer.clockSource;
    data.bit(10)    = vtimer.irqLine;
    data.bit(11)    = vtimer.reachedTarget;
    data.bit(12)    = vtimer.reachedSaturate;
    data.bit(13,15) = vtimer.unknown;
    vtimer.reachedTarget   = 0;
    vtimer.reachedSaturate = 0;
  }

  if(address == 0x1f80'1118) {
    data.bit(0,15) = vtimer.target;
  }

  if(address == 0x1f80'1120) {
    data.bit(0,15) = ftimer.counter;
  }

  if(address == 0x1f80'1124) {
    data.bit( 0)    = ftimer.synchronizeEnable;
    data.bit( 1, 2) = ftimer.synchronizeMode;
    data.bit( 3)    = ftimer.resetMode;
    data.bit( 4)    = ftimer.irqOnTarget;
    data.bit( 5)    = ftimer.irqOnSaturate;
    data.bit( 6)    = ftimer.irqRepeat;
    data.bit( 7)    = ftimer.irqMode;
    data.bit( 8, 9) = ftimer.clockSource;
    data.bit(10)    = ftimer.irqLine;
    data.bit(11)    = ftimer.reachedTarget;
    data.bit(12)    = ftimer.reachedSaturate;
    data.bit(13,15) = ftimer.unknown;
    ftimer.reachedTarget   = 0;
    ftimer.reachedSaturate = 0;
  }

  if(address == 0x1f80'1128) {
    data.bit(0,15) = ftimer.counter;
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

  if(address == 0x1f80'1100) {
    htimer.counter = data.bit(0,15);
  }

  if(address == 0x1f80'1104) {
    htimer.synchronizeEnable = data.bit( 0);
    htimer.synchronizeMode   = data.bit( 1, 2);
    htimer.resetMode         = data.bit( 3);
    htimer.irqOnTarget       = data.bit( 4);
    htimer.irqOnSaturate     = data.bit( 5);
    htimer.irqRepeat         = data.bit( 6);
    htimer.irqMode           = data.bit( 7);
    htimer.clockSource       = data.bit( 8, 9);
    htimer.irqLine           = data.bit(10);
    htimer.reachedTarget     = data.bit(11);
    htimer.reachedSaturate   = data.bit(12);
    htimer.unknown           = data.bit(13,15);
  }

  if(address == 0x1f80'1108) {
    htimer.target = data.bit(0,15);
  }

  if(address == 0x1f80'1110) {
    vtimer.counter = data.bit(0,15);
  }

  if(address == 0x1f80'1114) {
    vtimer.synchronizeEnable = data.bit( 0);
    vtimer.synchronizeMode   = data.bit( 1, 2);
    vtimer.resetMode         = data.bit( 3);
    vtimer.irqOnTarget       = data.bit( 4);
    vtimer.irqOnSaturate     = data.bit( 5);
    vtimer.irqRepeat         = data.bit( 6);
    vtimer.irqMode           = data.bit( 7);
    vtimer.clockSource       = data.bit( 8, 9);
    vtimer.irqLine           = data.bit(10);
    vtimer.reachedTarget     = data.bit(11);
    vtimer.reachedSaturate   = data.bit(12);
    vtimer.unknown           = data.bit(13,15);
  }

  if(address == 0x1f80'1118) {
    vtimer.target = data.bit(0,15);
  }

  if(address == 0x1f80'1120) {
    ftimer.counter = data.bit(0,15);
  }

  if(address == 0x1f80'1124) {
    ftimer.synchronizeEnable = data.bit( 0);
    ftimer.synchronizeMode   = data.bit( 1, 2);
    ftimer.resetMode         = data.bit( 3);
    ftimer.irqOnTarget       = data.bit( 4);
    ftimer.irqOnSaturate     = data.bit( 5);
    ftimer.irqRepeat         = data.bit( 6);
    ftimer.irqMode           = data.bit( 7);
    ftimer.clockSource       = data.bit( 8, 9);
    ftimer.irqLine           = data.bit(10);
    ftimer.reachedTarget     = data.bit(11);
    ftimer.reachedSaturate   = data.bit(12);
    ftimer.unknown           = data.bit(13,15);
  }

  if(address == 0x1f80'1128) {
    ftimer.target = data.bit(0,15);
  }
}

auto Timer::writeWord(u32 address, u32 data) -> void {
  writeHalf(address & ~3 | 0, data >>  0);
  writeHalf(address & ~3 | 2, data >> 16);
}
