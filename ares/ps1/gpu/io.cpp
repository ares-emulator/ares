auto GPU::readByte(u32 address) -> u8 {
  print("* read byte ", hex(address, 8L), "\n");
  return 0;
}

auto GPU::readHalf(u32 address) -> u16 {
  print("* read half ", hex(address, 8L), "\n");
  return 0;
}

auto GPU::readWord(u32 address) -> u32 {
  uint32 data;
  if(address == 0x1f80'1810) data = readGP0();
  if(address == 0x1f80'1814) data = readGP1();
  return data;
}

auto GPU::writeByte(u32 address, u8 value) -> void {
  print("* write byte ", hex(address, 8L), " = ", hex(value, 2L), "\n");
}

auto GPU::writeHalf(u32 address, u16 value) -> void {
  print("* write half ", hex(address, 8L), " = ", hex(value, 4L), "\n");
}

auto GPU::writeWord(u32 address, u32 data) -> void {
  if(address == 0x1f80'1810) writeGP0(data);
  if(address == 0x1f80'1814) writeGP1(data);
}
