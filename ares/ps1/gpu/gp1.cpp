auto GPU::readGP1() -> u32 {
  uint32 data;
  data.bit( 0, 3) = io.texturePageBaseX;
  data.bit( 4)    = io.texturePageBaseY;
  data.bit( 5, 6) = io.semiTransparency;
  data.bit( 7, 8) = io.textureDepth;
  data.bit( 9)    = io.dithering;
  data.bit(10)    = io.drawToDisplay;
  data.bit(11)    = io.forceMaskBit;
  data.bit(12)    = io.checkMaskBit;
  data.bit(13)    = io.interlace == 0;  //todo
  data.bit(14)    = io.reverseFlag;
  data.bit(15)    = io.textureDisable;
  data.bit(16)    = io.horizontalResolution.bit(2);
  data.bit(17,18) = io.horizontalResolution.bit(0,1);
  data.bit(19)    = io.verticalResolution;
  data.bit(20)    = io.videoMode;
  data.bit(21)    = io.colorDepth;
  data.bit(22)    = io.interlace;
  data.bit(23)    = io.displayDisable;
  data.bit(24)    = io.interrupt;
  data.bit(25)    = io.dmaDirection > 0;  //todo
  data.bit(26)    = 1;  //ready to receive command
  data.bit(27)    = 1;  //ready to send VRAM to CPU
  data.bit(28)    = 1;  //ready to receive DMA block
  data.bit(29,30) = io.dmaDirection;
  data.bit(31)    = (io.vcounter >= 240) || (io.interlace && !(io.vcounter & 1)) ? 0 : 1;
  return data;
}

auto GPU::writeGP1(u32 value) -> void {
   uint8 command = value >> 24;
  uint24 data    = value >>  0;

//print("* GP1(", hex(command, 2L), ") = ", hex(data, 6L), "\n");

  //soft reset
  if(command == 0x00) {
    queue.gp0.reset();
    return;
  }

  //reset command buffer
  if(command == 0x01) {
    queue.gp0.reset();
    return;
  }

  //acknowledge interrupt
  if(command == 0x02) {
    io.interrupt = 0;
    return;
  }

  //display disable
  if(command == 0x03) {
    io.displayDisable = data.bit(0);
    return;
  }

  //dma direction
  if(command == 0x04) {
    io.dmaDirection = data.bit(0,1);
    return;
  }

  //start of display area
  if(command == 0x05) {
    io.displayStartX = data.bit( 0, 9) & ~1;  //16-bit align
    io.displayStartY = data.bit(10,18);
    return;
  }

  //horizontal display range
  if(command == 0x06) {
    io.displayRangeX1 = data.bit( 0,11);
    io.displayRangeX2 = data.bit(12,23);
    return;
  }

  //vertical display range
  if(command == 0x07) {
    io.displayRangeY1 = data.bit( 0, 9);
    io.displayRangeY2 = data.bit(10,19);
    return;
  }

  //display mode
  if(command == 0x08) {
    io.horizontalResolution.bit(0,1) = data.bit(0,1);
    io.verticalResolution            = data.bit(2);
    io.videoMode                     = data.bit(3);
    io.colorDepth                    = data.bit(4);
    io.interlace                     = data.bit(5);
    io.horizontalResolution.bit(2)   = data.bit(6);
    io.reverseFlag                   = data.bit(7);
    return;
  }

//print("* GP1(", hex(command, 2L), ") = ", hex(data, 6L), "\n");
}
