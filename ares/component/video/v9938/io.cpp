auto V9938::status() -> n8 {
  io.controlLatch = 0;
  n8 data;

  switch(io.statusIndex) {

  case 0:
    data.bit(0,4) = sprite.io.overflowIndex;
    data.bit(5)   = sprite.io.collision;
    data.bit(6)   = sprite.io.overflow;
    data.bit(7)   = virq.pending;

    sprite.io.overflowIndex = 0b11111;
    sprite.io.collision = 0;
    sprite.io.overflow = 0;
    virq.pending = 0;
    poll();
    return data;

  case 1:
    data.bit(0) = hirq.pending;
    data.bit(7) = lirq.pending;
    hirq.pending = 0;
    lirq.pending = 0;
    poll();
    return data;

  case 2:
    data.bit(0) = op.executing;
    data.bit(1) = field();
    data.bit(2) = 1;
    data.bit(3) = 1;
    data.bit(4) = op.found;
    data.bit(5) = io.hcounter >= 256;
    data.bit(6) = io.vcounter >= vlines();
    data.bit(7) = op.ready;
    return data;

  case 4:
    data.bit(1,7) = 0b1111111;
    return data;

  case 6:
    data.bit(1,7) = 0b1111111;
    return data;

  case 7:
    data.bit(0,7) = op.cr;
    op.ready = 0;
    return data;

  case 8:
    data.bit(0,7) = op.match.bit(0,7);
    return data;

  case 9:
    data.bit(0)   = op.match.bit(8);
    data.bit(1,7) = 0b1111111;
    return data;

  }

  return data;
}

auto V9938::data() -> n8 {
  io.controlLatch = 0;
  n17 address = io.ramBank << 14 | io.controlValue.bit(0,13)++;
  if(!io.controlValue.bit(0,13)) io.ramBank++;  //unconfirmed
  n8 data = io.ramLatch;
  if(io.ramSelect == 0) {
    io.ramLatch = vram.read(address);
  } else {
    io.ramLatch = xram.read(address);
  }
  return data;
}

//

auto V9938::data(n8 data) -> void {
  io.controlLatch = 0;
  n17 address = io.ramBank << 14 | io.controlValue.bit(0,13)++;
  if(!io.controlValue.bit(0,13)) io.ramBank++;  //unconfirmed
  if(io.ramSelect == 0) {
    vram.write(address, data);
  } else {
    xram.write(address, data);
  }
}

auto V9938::control(n8 data) -> void {
  io.controlValue.byte(io.controlLatch++) = data;
  if(io.controlLatch) return;
  if(io.controlValue.bit(15)) {
    return register(io.controlValue.bit(8,13), io.controlValue.bit(0,7));
  }
  if(!io.controlValue.bit(14)) V9938::data();  //read-ahead
}

auto V9938::palette(n8 data) -> void {
  io.paletteValue.byte(io.paletteLatch++) = data;
  if(io.paletteLatch) return;
  pram[io.paletteIndex].bit(0,2) = io.paletteValue.bit(0, 2);  //B
  pram[io.paletteIndex].bit(3,5) = io.paletteValue.bit(4, 6);  //R
  pram[io.paletteIndex].bit(6,8) = io.paletteValue.bit(8,10);  //G
  io.paletteIndex++;
}

auto V9938::register(n8 data) -> void {
  //indirect register writes cannot change the indirect register index setting
  if(io.registerIndex != 0x11) register(io.registerIndex, data);
  if(!io.registerFixed) io.registerIndex++;
}

auto V9938::register(n6 register, n8 data) -> void {
  switch(register) {
  case 0x00:
    io.videoMode.bit(2) = data.bit(1);
    io.videoMode.bit(3) = data.bit(2);
    io.videoMode.bit(4) = data.bit(3);
    hirq.enable = data.bit(4);
    lirq.enable = data.bit(5);
    dac.io.digitize = data.bit(6);
    hirq.pending &= hirq.enable;
    lirq.pending &= lirq.enable;
    poll();
    return;

  case 0x01:
    sprite.io.zoom = data.bit(0);
    sprite.io.size = data.bit(1);
    io.videoMode.bit(1) = data.bit(3);
    io.videoMode.bit(0) = data.bit(4);
    virq.enable = data.bit(5);
    dac.io.enable = data.bit(6);
    virq.pending &= virq.enable;
    poll();
    return;

  case 0x02:
    background.io.nameTableAddress.bit(10,16) = data.bit(0,6);
    return;

  case 0x03:
    background.io.colorTableAddress.bit(6,13) = data.bit(0,7);
    return;

  case 0x04:
    background.io.patternTableAddress.bit(11,16) = data.bit(0,5);
    return;

  case 0x05:
    sprite.io.nameTableAddress.bit(7,14) = data.bit(0,7);
    return;

  case 0x06:
    sprite.io.patternTableAddress.bit(11,16) = data.bit(0,5);
    return;

  case 0x07:
    io.colorBackground = data.bit(0,3);
    io.colorForeground = data.bit(4,7);
    return;

  case 0x08:
    dac.io.grayscale = data.bit(0);
    sprite.io.disable = data.bit(1);
    return;

  case 0x09:
    io.timing = data.bit(1);
    io.interlace = data.bit(3);
    io.overscan = data.bit(7);
    return;

  case 0x0a:
    background.io.colorTableAddress.bit(14,16) = data.bit(0,2);
    return;

  case 0x0b:
    sprite.io.nameTableAddress.bit(15,16) = data.bit(0,1);
    return;

  case 0x0c:
    io.blinkColorBackground = data.bit(0,3);
    io.blinkColorForeground = data.bit(4,7);
    return;

  case 0x0d:
    io.blinkPeriodBackground = data.bit(0,3);
    io.blinkPeriodForeground = data.bit(4,7);
    return;

  case 0x0e:
    io.ramBank = data.bit(0,2);
    return;

  case 0x0f:
    io.statusIndex = data.bit(0,3);
    return;

  case 0x10:
    io.paletteIndex = data.bit(0,3);
    return;

  case 0x11:
    io.registerIndex = data.bit(0,5);
    io.registerFixed = data.bit(7);
    return;

  case 0x12:
    background.io.hadjust = data.bit(0,3);
    background.io.vadjust = data.bit(4,7);
    return;

  case 0x13:
    hirq.coincidence = data;
    return;

  case 0x17:
    background.io.vscroll = data;
    return;

  case 0x20: op.sx.bit(0,7) = data.bit(0,7); return;
  case 0x21: op.sx.bit(8)   = data.bit(0);   return;
  case 0x22: op.sy.bit(0,7) = data.bit(0,7); return;
  case 0x23: op.sy.bit(8,9) = data.bit(0,1); return;
  case 0x24: op.dx.bit(0,7) = data.bit(0,7); return;
  case 0x25: op.dx.bit(8)   = data.bit(0);   return;
  case 0x26: op.dy.bit(0,7) = data.bit(0,7); return;
  case 0x27: op.dy.bit(8,9) = data.bit(0,1); return;
  case 0x28: op.nx.bit(0,7) = data.bit(0,7); return;
  case 0x29: op.nx.bit(8)   = data.bit(0);   return;
  case 0x2a: op.ny.bit(0,7) = data.bit(0,7); return;
  case 0x2b: op.ny.bit(8,9) = data.bit(0,1); return;
  case 0x2c: op.cr.bit(0,7) = data.bit(0,7); op.ready = 0; return;

  case 0x2d:
    op.maj = data.bit(0);
    op.eq  = data.bit(1);
    op.dix = data.bit(2);
    op.diy = data.bit(3);
    op.mxs = data.bit(4);
    op.mxd = data.bit(5);
    io.ramSelect = data.bit(6);
    return;

  case 0x2e:
    command(data);
    return;
  }
}
