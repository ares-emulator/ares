auto GPU::readByte(u32 address) -> u8 {
  return 0;
}

auto GPU::readHalf(u32 address) -> u16 {
  return 0;
}

auto GPU::readWord(u32 address) -> u32 {
  uint32 data = 0;

  //GPUREAD
  if(address == 0x1f80'1810) {
  }

  //GPUSTAT
  if(address == 0x1f80'1814) {
    switch(io.dmaDirection) {
    case 0: data.bit(25) = 0; break;
    case 1: data.bit(25) = 1; break;
    case 2: data.bit(25) = 1; break;
    case 3: data.bit(25) = 1; break;
    }
    data.bit(26)    = 1;  //ready to receive command
    data.bit(27)    = 1;  //ready to send VRAM to CPU
    data.bit(28)    = 1;  //ready to receive DMA block
    data.bit(29,30) = io.dmaDirection;
  }

  return data;
}

auto GPU::writeByte(u32 address, u8 value) -> void {
}

auto GPU::writeHalf(u32 address, u16 value) -> void {
}

auto GPU::writeWord(u32 address, u32 value) -> void {
  uint32 data = value;

  //GP0
  if(address == 0x1f80'1810) {
  }

  //GP1
  if(address == 0x1f80'1814) {
    u8 command = data >> 24;
    if(command == 0x04) {
      io.dmaDirection = data.bit(0,1);
    }
  }
}
