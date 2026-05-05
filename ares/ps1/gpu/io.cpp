auto GPU::canReadDMA() -> bool {
  return (readGP1() >> 25) & 1;
}

auto GPU::canWriteDMA() -> bool {
  return (readGP1() >> 25) & 1;
}

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
  if(address == 0x1f80'1810) return data = readGP0();
  if(address == 0x1f80'1814) return data = readGP1();
  debug(unhandled, "GPU::readWord(", hex(address, 8L), ") -> ", hex(data, 8L));
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
  if(address == 0x1f80'1810) return writeGP0(data);
  if(address == 0x1f80'1814) return writeGP1(data);
  debug(unhandled, "GPU::writeWord(", hex(address, 8L), ", ", hex(data, 8L), ")");
}
