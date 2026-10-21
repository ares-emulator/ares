auto SPU::captureVolume(u32 channel, s16 volume) -> void {
  u16 address = channel * 0x400 + capture.address;
  if(transfer.type.bit(1) || transfer.type.bit(2)) return writeRAM(address, volume);
  ram.writeHalf(address, volume);
}
