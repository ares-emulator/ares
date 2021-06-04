auto VDP::vcounterQuery() -> n8 {
  if(Region::NTSCJ() || Region::NTSCU()) {
    switch(videoMode()) {
    default:     return io.vcounter <= 218 ? io.vcounter : io.vcounter - 6;  //256x192
    case 0b1011: return io.vcounter <= 234 ? io.vcounter : io.vcounter - 6;  //256x224
    case 0b1110: return io.vcounter;  //256x240
    }
  }

  if(Region::PAL()) {
    switch(videoMode()) {
    default:     return io.vcounter <= 242 ? io.vcounter : io.vcounter - 57;  //256x192
    case 0b1011: return io.vcounter <= 258 ? io.vcounter : io.vcounter - 57;  //256x224
    case 0b1110: return io.vcounter <= 266 ? io.vcounter : io.vcounter - 56;  //256x240
    }
  }

  unreachable;
}

auto VDP::hcounterQuery() -> n8 {
  return latch.hcounter - 94 >> 2;
}

auto VDP::hcounterLatch() -> void {
  latch.hcounter = io.hcounter;
}

auto VDP::ccounter() -> n12 {
  return io.ccounter;
}

auto VDP::data() -> n8 {
  latch.control = 0;

  auto data = latch.vram;
  latch.vram = vram[io.address++];
  return data;
}

auto VDP::status() -> n8 {
  latch.control = 0;

  n8 data;
  data.bit(0,4) = sprite.io.overflowIndex;
  data.bit(5)   = sprite.io.collision;
  data.bit(6)   = sprite.io.overflow;
  data.bit(7)   = irq.frame.pending;

  sprite.io.overflowIndex = 0b11111;
  sprite.io.collision = 0;
  sprite.io.overflow = 0;
  irq.frame.pending = 0;
  irq.line.pending = 0;
  irq.poll();

  return data;
}

auto VDP::data(n8 data) -> void {
  latch.control = 0;

  if(io.code <= 2) {
    vram[io.address] = data;
  } else {
    if(Mode::MasterSystem()) {
      //writes immediate store 6-bits into CRAM
      cram[io.address] = data.bit(0,5);
    }
    if(Mode::GameGear()) {
      //even writes store 8-bit data into a latch; odd writes store 12-bits into CRAM
      if(io.address.bit(0) == 0) {
        latch.cram = data;
      } else {
        cram[io.address >> 1] = data.bit(0,3) << 8 | latch.cram;
      }
    }
  }
  io.address++;
}

auto VDP::control(n8 data) -> void {
  if(latch.control == 0) {
    latch.control = 1;
    io.address.bit(0,7) = data.bit(0,7);
    return;
  }

  latch.control = 0;
  io.address.bit(8,13) = data.bit(0,5);
  io.code.bit(0,1)     = data.bit(6,7);

  if(io.code == 0) {
    latch.vram = vram[io.address++];
  }

  if(io.code == 2) {
    registerWrite(io.address.bit(11,8), io.address.bit(7,0));
  }
}

auto VDP::registerWrite(n4 address, n8 data) -> void {
  debugger.io(address, data);

  switch(address) {
  case 0x0:  //mode control 1
    dac.io.externalSync       = data.bit(0);
    io.videoMode.bit(1)       = data.bit(1);
    io.videoMode.bit(3)       = data.bit(2);
    sprite.io.shift           = data.bit(3);
    irq.line.enable           = data.bit(4);
    dac.io.leftClip           = data.bit(5);
    background.io.hscrollLock = data.bit(6);
    background.io.vscrollLock = data.bit(7);
    irq.line.pending &= irq.line.enable;
    irq.poll();
    return;

  case 0x1:  //mode control 2
    sprite.io.zoom       = data.bit(0);
    sprite.io.size       = data.bit(1);
    io.videoMode.bit(2)  = data.bit(3);
    io.videoMode.bit(0)  = data.bit(4);
    irq.frame.enable     = data.bit(5);
    io.displayEnable     = data.bit(6);
    irq.frame.pending &= irq.frame.enable;
    irq.poll();
    return;

  case 0x2:  //name table base address
    background.io.nameTableAddress = data.bit(0,3);
    return;

  case 0x3:  //color table base address
    background.io.colorTableAddress = data.bit(0,7);
    return;

  case 0x4:  //pattern table base address
    background.io.patternTableAddress = data.bit(0,2);
    return;

  case 0x5:  //sprite attribute table base address
    sprite.io.attributeTableAddress = data.bit(0,6);
    return;

  case 0x6:  //sprite pattern table base address
    sprite.io.patternTableAddress = data.bit(0,2);
    return;

  case 0x7:  //backdrop color
    dac.io.backdropColor = data.bit(0,3);
    return;

  case 0x8:  //horizontal scroll offset
    background.io.hscroll = data.bit(0,7);
    return;

  case 0x9:  //vertical scroll offset
    background.io.vscroll = data.bit(0,7);
    return;

  case 0xa:  //line counter
    irq.line.coincidence = data.bit(0,7);
    return;
  }
}
