auto GPU::readByte(u32 address) -> u8 {
  print("* read byte ", hex(address, 8L), "\n");
  return 0;
}

auto GPU::readHalf(u32 address) -> u16 {
  print("* read half ", hex(address, 8L), "\n");
  return 0;
}

auto GPU::readWord(u32 address) -> u32 {
  uint32 data = 0;

  //GPUREAD
  if(address == 0x1f80'1810) {
  }

  //GPUSTAT
  if(address == 0x1f80'1814) {
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
    data.bit(31)    = io.interlace ? (bool)io.field : io.vcounter & 1;
  }

  return data;
}

auto GPU::writeByte(u32 address, u8 value) -> void {
  print("* write byte ", hex(address, 8L), " = ", hex(value, 2L), "\n");
}

auto GPU::writeHalf(u32 address, u16 value) -> void {
  print("* write half ", hex(address, 8L), " = ", hex(value, 4L), "\n");
}

auto GPU::writeWord(u32 address, u32 value) -> void {
  uint32 data = value;

  //GP0
  if(address == 0x1f80'1810) {
    gp0(data);
  }

  //GP1
  if(address == 0x1f80'1814) {
    gp1(data);
  }
}
