auto VDP::vcounterQuery() -> n8 {
  if(Region::NTSCJ() || Region::NTSCU()) {
    switch(mode()) {
    default:     return io.vcounter <= 218 ? io.vcounter : io.vcounter - 6;  //256x192
    case 0b1011: return io.vcounter <= 234 ? io.vcounter : io.vcounter - 6;  //256x224
    case 0b1110: return io.vcounter;  //256x240
    }
  }

  if(Region::PAL()) {
    switch(mode()) {
    default:     return io.vcounter <= 242 ? io.vcounter : io.vcounter - 57;  //256x192
    case 0b1011: return io.vcounter <= 258 ? io.vcounter : io.vcounter - 57;  //256x224
    case 0b1110: return io.vcounter <= 266 ? io.vcounter : io.vcounter - 56;  //256x240
    }
  }

  unreachable;
}

auto VDP::hcounterQuery() -> n8 {
  return io.pcounter - 94 >> 2;
}

auto VDP::hcounterLatch() -> void {
  io.pcounter = io.hcounter;
}

auto VDP::ccounter() -> n12 {
  return io.ccounter;
}

auto VDP::data() -> n8 {
  io.controlLatch = 0;

  auto data = io.vramLatch;
  io.vramLatch = vram[io.address++];
  return data;
}

auto VDP::status() -> n8 {
  io.controlLatch = 0;

  n8 data;
  data.bit(0,4) = sprite.io.fifth;
  data.bit(5)   = sprite.io.collision;
  data.bit(6)   = sprite.io.overflow;
  data.bit(7)   = irq.frame.pending;

  sprite.io.fifth = 0;
  sprite.io.collision = 0;
  sprite.io.overflow = 0;
  irq.frame.pending = 0;
  irq.line.pending = 0;

  return data;
}

auto VDP::data(n8 data) -> void {
  io.controlLatch = 0;

  if(io.code <= 2) {
    vram[io.address++] = data;
  } else {
    if(Model::MasterSystem() || Model::GameGearMS()) {
      cram[io.address++ & 0x1f] = data;
    } else if(Model::GameGear()) {
      cram[io.address++ & 0x3f] = data;
    }
  }
}

auto VDP::control(n8 data) -> void {
  if(io.controlLatch == 0) {
    io.controlLatch = 1;
    io.address.bit(0,7) = data.bit(0,7);
    return;
  }

  io.controlLatch = 0;
  io.address.bit(8,13) = data.bit(0,5);
  io.code.bit(0,1)     = data.bit(6,7);

  if(io.code == 0) {
    io.vramLatch = vram[io.address++];
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
    io.mode.bit(1)            = data.bit(1);
    io.mode.bit(3)            = data.bit(2);
    sprite.io.shift           = data.bit(3);
    irq.line.enable           = data.bit(4);
    dac.io.leftClip           = data.bit(5);
    background.io.hscrollLock = data.bit(6);
    background.io.vscrollLock = data.bit(7);
    return;

  case 0x1:  //mode control 2
    sprite.io.zoom       = data.bit(0);
    sprite.io.size       = data.bit(1);
    io.mode.bit(2)       = data.bit(3);
    io.mode.bit(0)       = data.bit(4);
    irq.frame.enable     = data.bit(5);
    dac.io.displayEnable = data.bit(6);
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
    irq.line.counter = data.bit(0,7);
    return;
  }
}
