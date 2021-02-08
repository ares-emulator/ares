//GPUSTAT
auto GPU::readGP1() -> u32 {
  n32 data;
  data.bit( 0, 3) = io.texturePageBaseX;
  data.bit( 4)    = io.texturePageBaseY;
  data.bit( 5, 6) = io.semiTransparency;
  data.bit( 7, 8) = io.textureDepth;
  data.bit( 9)    = io.dithering;
  data.bit(10)    = io.drawToDisplay;
  data.bit(11)    = io.forceMaskBit;
  data.bit(12)    = io.checkMaskBit;
  data.bit(13)    = !io.field || !io.interlace;
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
  data.bit(26)    = io.pcounter == 0;  //ready to receive command
  data.bit(27)    = io.mode != Mode::CopyToVRAM;
  data.bit(28)    = io.pcounter == 0;  //ready to receive DMA block
  data.bit(29,30) = io.dmaDirection;
  data.bit(31)    = !vblank() && (interlace() ? !io.field : io.vcounter & 1);
  return data;
}

auto GPU::writeGP1(u32 value) -> void {
//thread.fifo.await_empty();

  n8  command = value >> 24;
  n24 data    = value >>  0;

//print("* GP1(", hex(command, 2L), ") = ", hex(data, 6L), "\n");

  //soft reset
  if(command == 0x00) {
    //GP0(e1)
    io.texturePageBaseX = 0;
    io.texturePageBaseY = 0;
    io.semiTransparency = 0;
    io.textureDepth = 0;
    io.dithering = 0;
    io.drawToDisplay = 0;
    io.textureDisable = 0;
    io.textureFlipX = 0;
    io.textureFlipY = 0;

    //GP0(e2)
    io.textureWindowMaskX = 0;
    io.textureWindowMaskY = 0;
    io.textureWindowOffsetX = 0;
    io.textureWindowOffsetY = 0;

    //GP0(e3)
    io.drawingAreaOriginX1 = 0;
    io.drawingAreaOriginY1 = 0;

    //GP0(e4)
    io.drawingAreaOriginX2 = 0;
    io.drawingAreaOriginY2 = 0;

    //GP0(e5)
    io.drawingAreaOffsetX = 0;
    io.drawingAreaOffsetY = 0;

    //GP0(e6)
    io.forceMaskBit = 0;
    io.checkMaskBit = 0;

    //GP1(01)
    queue.gp0.reset();

    //GP1(02)
    io.interrupt = 0;
    interrupt.lower(Interrupt::GPU);

    //GP1(03)
    io.displayDisable = 1;

    //GP1(04)
    io.dmaDirection = 0;

    //GP1(05)
    io.displayStartX = 0;
    io.displayStartY = 0;

    //GP1(06)
    io.displayRangeX1 = 512;
    io.displayRangeX2 = 512 + 256 * 10;

    //GP1(07)
    io.displayRangeY1 =  16;
    io.displayRangeY2 =  16 + 240;

    //GP1(08)
    io.horizontalResolution = 1;
    io.verticalResolution = 0;
    io.videoMode = 0;
    io.colorDepth = 0;
    io.interlace = 0;
    io.reverseFlag = 0;

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
    interrupt.lower(Interrupt::GPU);
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

  //get GPU information
  if(command >= 0x10 && command <= 0x1f) {
    io.mode = Mode::Status;
    data &= 0xf;

    //GP1(e2)
    if(data == 0x2) {
      io.status.bit( 0, 4) = io.textureWindowMaskX;
      io.status.bit( 5, 9) = io.textureWindowMaskY;
      io.status.bit(10,14) = io.textureWindowOffsetX;
      io.status.bit(15,19) = io.textureWindowOffsetY;
      io.status.bit(20,23) = 0;
      return;
    }

    //GP1(e3)
    if(data == 0x3) {
      io.status.bit( 0, 9) = io.drawingAreaOriginX1;
      io.status.bit(10,19) = io.drawingAreaOriginY1;
      io.status.bit(20,23) = 0;
      return;
    }

    //GP1(e4)
    if(data == 0x4) {
      io.status.bit( 0, 9) = io.drawingAreaOriginX2;
      io.status.bit(10,19) = io.drawingAreaOriginY2;
      io.status.bit(20,23) = 0;
      return;
    }

    //GP1(e5)
    if(data == 0x5) {
      io.status.bit(0, 10) = io.drawingAreaOffsetX;
      io.status.bit(11,21) = io.drawingAreaOffsetY;
      io.status.bit(22,23) = 0;
      return;
    }

    //GP1(e7): GPU type
    if(data == 0x7) {
      io.status.bit(0,23) = 2;
      return;
    }

    //GP1(e8): unknown
    if(data == 0x8) {
      io.status.bit(0,23) = 0;
      return;
    }

    return;
  }

  debug(unimplemented, "GP1(", hex(command, 2L), ") = ", hex(data, 6L));
}
