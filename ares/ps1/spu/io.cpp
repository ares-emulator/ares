auto SPU::readByte(u32 address) -> u8 {
  return 0;
}

auto SPU::readHalf(u32 address) -> u16 {
  uint16 data = 0;

  //SPURAMCNT
  if(address == 0x1f80'1dac) {
    data.bit(1,3) = io.ramTransferType;
  }

  //SPUSTAT
  if(address == 0x1f80'1dae) {
    data.bit(0)   = io.cdAudioEnable;
    data.bit(1)   = io.externalAudioEnable;
    data.bit(2)   = io.cdAudioReverb;
    data.bit(3)   = io.externalAudioReverb;
    data.bit(4,5) = io.ramTransferMode;
    data.bit(7)   = io.ramTransferMode.bit(1);  //unverified
  }

  return data;
}

auto SPU::readWord(u32 address) -> u32 {
  return 0;
}

auto SPU::writeByte(u32 address, u8 value) -> void {
}

auto SPU::writeHalf(u32 address, u16 value) -> void {
  uint16 data = value;

  //SPUCNT
  if(address == 0x1f80'1daa) {
    io.cdAudioEnable       = data.bit(0);
    io.externalAudioEnable = data.bit(1);
    io.cdAudioReverb       = data.bit(2);
    io.externalAudioReverb = data.bit(3);
    io.ramTransferMode     = data.bit(4,5);
  }

  //SPURAMCHT
  if(address == 0x1f80'1dac) {
    io.ramTransferType = data.bit(1,3);
  }
}

auto SPU::writeWord(u32 address, u32 value) -> void {
}
