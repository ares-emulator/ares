auto CPU::Interrupt::poll() -> void {
  line = 0;
  line |= vblank.stat & vblank.mask;
  line |= gpu.stat & gpu.mask;
  line |= cdrom.stat & cdrom.mask;
  line |= dma.stat & dma.mask;
  line |= timer0.stat & timer0.mask;
  line |= timer1.stat & timer1.mask;
  line |= timer2.stat & timer2.mask;
  line |= peripheral.stat & peripheral.mask;
  line |= sio.stat & sio.mask;
  line |= spu.stat & spu.mask;
  line |= pio.stat & pio.mask;
  self.scc.cause.interruptPending.bit(2) = line;
}

auto CPU::Interrupt::pulse(uint source) -> void {
  raise(source);
  lower(source);
}

auto CPU::Interrupt::raise(uint source) -> void {
  if(source ==  0 && !vblank.line) vblank.line = vblank.stat = 1;
  if(source ==  1 && !gpu.line) gpu.line = gpu.stat = 1;
  if(source ==  2 && !cdrom.line) cdrom.line = cdrom.stat = 1;
  if(source ==  3 && !dma.line) dma.line = dma.stat = 1;
  if(source ==  4 && !timer0.line) timer0.line = timer0.stat = 1;
  if(source ==  5 && !timer1.line) timer1.line = timer1.stat = 1;
  if(source ==  6 && !timer2.line) timer2.line = timer2.stat = 1;
  if(source ==  7 && !peripheral.line) peripheral.line = peripheral.stat = 1;
  if(source ==  8 && !sio.line) sio.line = sio.stat = 1;
  if(source ==  9 && !spu.line) spu.line = spu.stat = 1;
  if(source == 10 && !pio.line) pio.line = pio.stat = 1;
  poll();
}

auto CPU::Interrupt::lower(uint source) -> void {
  if(source ==  0) vblank.line = 0;
  if(source ==  1) gpu.line = 0;
  if(source ==  2) cdrom.line = 0;
  if(source ==  3) dma.line = 0;
  if(source ==  4) timer0.line = 0;
  if(source ==  5) timer1.line = 0;
  if(source ==  6) timer2.line = 0;
  if(source ==  7) peripheral.line = 0;
  if(source ==  8) sio.line = 0;
  if(source ==  9) spu.line = 0;
  if(source == 10) pio.line = 0;
  poll();
}

auto CPU::Interrupt::readByte(u32 address) -> u8 {
  uint8 data = 0;
  print("* read byte ", hex(address, 8L), "\n");
  return data;
}

auto CPU::Interrupt::readHalf(u32 address) -> u16 {
  uint16 data = 0;

  //I_STAT
  if(address == 0x1f80'1070) {
    data.bit( 0) = vblank.stat;
    data.bit( 1) = gpu.stat;
    data.bit( 2) = cdrom.stat;
    data.bit( 3) = dma.stat;
    data.bit( 4) = timer0.stat;
    data.bit( 5) = timer1.stat;
    data.bit( 6) = timer2.stat;
    data.bit( 7) = peripheral.stat;
    data.bit( 8) = sio.stat;
    data.bit( 9) = spu.stat;
    data.bit(10) = pio.stat;
  }

  //I_MASK
  if(address == 0x1f80'1074) {
    data.bit( 0) = vblank.mask;
    data.bit( 1) = gpu.mask;
    data.bit( 2) = cdrom.mask;
    data.bit( 3) = dma.mask;
    data.bit( 4) = timer0.mask;
    data.bit( 5) = timer1.mask;
    data.bit( 6) = timer2.mask;
    data.bit( 7) = peripheral.mask;
    data.bit( 8) = sio.mask;
    data.bit( 9) = spu.mask;
    data.bit(10) = pio.mask;
  }

  return data;
}

auto CPU::Interrupt::readWord(u32 address) -> u32 {
  uint32 data = 0;
  data |= readHalf(address & ~3 | 0) <<  0;
  data |= readHalf(address & ~3 | 2) << 16;
  return data;
}

auto CPU::Interrupt::writeByte(u32 address, u8 value) -> void {
  uint8 data = value;
  print("* write byte ", hex(address, 8L), " = ", hex(value, 2L), "\n");
}

auto CPU::Interrupt::writeHalf(u32 address, u16 value) -> void {
  uint16 data = value;

  //I_STAT
  if(address == 0x1f80'1070) {
    if(!data.bit( 0)) vblank.stat = 0;
    if(!data.bit( 1)) gpu.stat = 0;
    if(!data.bit( 2)) cdrom.stat = 0;
    if(!data.bit( 3)) dma.stat = 0;
    if(!data.bit( 4)) timer0.stat = 0;
    if(!data.bit( 5)) timer1.stat = 0;
    if(!data.bit( 6)) timer2.stat = 0;
    if(!data.bit( 7)) peripheral.stat = 0;
    if(!data.bit( 8)) sio.stat = 0;
    if(!data.bit( 9)) spu.stat = 0;
    if(!data.bit(10)) pio.stat = 0;
    poll();
  }

  //I_MASK
  if(address == 0x1f80'1074) {
    vblank.mask     = data.bit( 0);
    gpu.mask        = data.bit( 1);
    cdrom.mask      = data.bit( 2);
    dma.mask        = data.bit( 3);
    timer0.mask     = data.bit( 4);
    timer1.mask     = data.bit( 5);
    timer2.mask     = data.bit( 6);
    peripheral.mask = data.bit( 7);
    sio.mask        = data.bit( 8);
    spu.mask        = data.bit( 9);
    pio.mask        = data.bit(10);
    poll();
  }
}

auto CPU::Interrupt::writeWord(u32 address, u32 data) -> void {
  writeHalf(address & ~3 | 0, data >>  0);
  writeHalf(address & ~3 | 2, data >> 16);
}
