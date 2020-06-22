auto GPU::readWord(u32 address) -> u32 {
  uint32 data = 0;

  //GPUREAD
  if(address == 0x1f80'1810) {
  }

  //GPUSTAT
  if(address == 0x1f80'1814) {
    data.bit(26) = 1;  //ready to receive command
    data.bit(27) = 1;  //ready to send VRAM to CPU
    data.bit(28) = 1;  //ready to receive DMA block
  }

  return data;
}

auto GPU::writeWord(u32 address, u32 value) -> void {
  uint32 data = value;

  //GP0
  if(address == 0x1f80'1810) {
  }

  //GP1
  if(address == 0x1f80'1814) {
  }
}
