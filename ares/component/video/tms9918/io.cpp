auto TMS9918::status() -> uint8 {
  io.controlLatch = 0;
  uint8 data = 0x00;
  data.bit(0,4) = io.spriteOverflowIndex;
  data.bit(5) = io.spriteCollision;
  data.bit(6) = io.spriteOverflow;
  data.bit(7) = io.irqLine;
  irq(io.irqLine = 0);
  return data;
}

auto TMS9918::data() -> uint8 {
  io.controlLatch = 0;
  uint14 address = io.controlValue.bit(0,13)++;
  uint8 data = io.vramLatch;
  io.vramLatch = vram.read(address);
  return data;
}

auto TMS9918::data(uint8 data) -> void {
  io.controlLatch = 0;
  uint14 address = io.controlValue.bit(0,13)++;
  vram.write(address, data);
}

auto TMS9918::control(uint8 data) -> void {
  io.controlValue.byte(io.controlLatch++) = data;
  if(io.controlLatch) return;
  if(io.controlValue.bit(15)) {
    return register(io.controlValue.bit(8,10), io.controlValue.bit(0,7));
  }
  if(!io.controlValue.bit(14)) TMS9918::data();  //read-ahead
}

auto TMS9918::register(uint3 register, uint8 data) -> void {
  switch(register) {
  case 0:
    io.externalInput = data.bit(0);
    io.videoMode.bit(2) = data.bit(1);
    break;
  case 1:
    io.spriteZoom = data.bit(0);
    io.spriteSize = data.bit(1);
    io.videoMode.bit(1) = data.bit(3);
    io.videoMode.bit(0) = data.bit(4);
    io.irqEnable = data.bit(5);
    io.displayEnable = data.bit(6);
    io.ramMode = data.bit(7);
    break;
  case 2:
    io.nameTableAddress = data.bit(0,3);
    break;
  case 3:
    io.colorTableAddress = data.bit(0,7);
    break;
  case 4:
    io.patternTableAddress = data.bit(0,2);
    break;
  case 5:
    io.spriteAttributeTableAddress = data.bit(0,6);
    break;
  case 6:
    io.spritePatternTableAddress = data.bit(0,2);
    break;
  case 7:
    io.colorBackground = data.bit(0,3);
    io.colorForeground = data.bit(4,7);
    break;
  }
}
