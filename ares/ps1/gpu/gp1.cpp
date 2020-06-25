auto GPU::gp1(u32 value) -> void {
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
