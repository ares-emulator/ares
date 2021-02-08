auto SPU::captureVolume(u32 channel, s16 volume) -> void {
  u16 address = channel * 0x400 + capture.address;
  writeRAM(address, volume);
}
