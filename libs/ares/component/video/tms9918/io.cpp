auto TMS9918::status() -> n8 {
  io.controlLatch = 0;

  n8 data = 0x00;
  data.bit(0,4) = sprite.io.overflowIndex;
  data.bit(5)   = sprite.io.collision;
  data.bit(6)   = sprite.io.overflow;
  data.bit(7)   = irqFrame.pending;

  sprite.io.overflowIndex = 0b11111;
  sprite.io.collision = 0;
  sprite.io.overflow = 0;
  irqFrame.pending = 0;
  poll();

  return data;
}

auto TMS9918::data() -> n8 {
  io.controlLatch = 0;

  n14 address = io.controlValue.bit(0,13)++;
  n8  data = io.vramLatch;
  io.vramLatch = vram.read(address);
  return data;
}

auto TMS9918::data(n8 data) -> void {
  io.controlLatch = 0;

  n14 address = io.controlValue.bit(0,13)++;
  vram.write(address, data);
}

auto TMS9918::control(n8 data) -> void {
  io.controlValue.byte(io.controlLatch++) = data;
  if(io.controlLatch) return;
  if(io.controlValue.bit(15)) {
    return register(io.controlValue.bit(8,10), io.controlValue.bit(0,7));
  }
  if(!io.controlValue.bit(14)) TMS9918::data();  //read-ahead
}

auto TMS9918::register(n3 register, n8 data) -> void {
  switch(register) {
  case 0:
    dac.io.externalSync = data.bit(0);
    io.videoMode.bit(2) = data.bit(1);
    break;

  case 1:
    sprite.io.zoom       = data.bit(0);
    sprite.io.size       = data.bit(1);
    io.videoMode.bit(1)  = data.bit(3);
    io.videoMode.bit(0)  = data.bit(4);
    irqFrame.enable      = data.bit(5);
    dac.io.displayEnable = data.bit(6);
    io.vramMode          = data.bit(7);
    poll();
    break;

  case 2:
    background.io.nameTableAddress = data.bit(0,3);
    break;

  case 3:
    background.io.colorTableAddress = data.bit(0,7);
    break;

  case 4:
    background.io.patternTableAddress = data.bit(0,2);
    break;

  case 5:
    sprite.io.attributeTableAddress = data.bit(0,6);
    break;

  case 6:
    sprite.io.patternTableAddress = data.bit(0,2);
    break;

  case 7:
    dac.io.colorBackground = data.bit(0,3);
    dac.io.colorForeground = data.bit(4,7);
    break;
  }
}
