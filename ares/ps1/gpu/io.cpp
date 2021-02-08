auto GPU::readDMA() -> u32 {
  return readWord(0x1f80'1810);
}

auto GPU::writeDMA(u32 data) -> void {
  return writeWord(0x1f80'1810, data);
}

auto GPU::readByte(u32 address) -> u32 {
  debug(unverified, "GPU::readByte(", hex(address, 8L), ")");
  return readWord(address & ~3) >> 8 * (address & 3);
}

auto GPU::readHalf(u32 address) -> u32 {
  debug(unverified, "GPU::readHalf(", hex(address, 8L), ")");
  return readWord(address & ~3) >> 8 * (address & 3);
}

auto GPU::readWord(u32 address) -> u32 {
  n32 data;
  if(address == 0x1f80'1810) data = readGP0();
  if(address == 0x1f80'1814) data = readGP1();
  return data;
}

auto GPU::writeByte(u32 address, u32 data) -> void {
  debug(unverified, "GPU::writeByte(", hex(address, 8L), ")");
  return writeWord(address & ~3, data << 8 * (address & 3));
}

auto GPU::writeHalf(u32 address, u32 data) -> void {
  debug(unverified, "GPU::writeHalf(", hex(address, 8L), ")");
  return writeWord(address & ~3, data << 8 * (address & 3));
}

auto GPU::writeWord(u32 address, u32 data) -> void {
  if(address == 0x1f80'1810) writeGP0(data);
  if(address == 0x1f80'1814) writeGP1(data);
}
