auto CPU::readByte(u32 address) -> maybe<u32> {
  return bus.readByte(address);
}

auto CPU::readHalf(u32 address) -> maybe<u32> {
  return bus.readHalf(address);
}

auto CPU::readWord(u32 address) -> maybe<u32> {
  return bus.readWord(address);
}

auto CPU::readWord(u32 address, u32 mask) -> maybe<u32> {
  return bus.readWord(address);
}

auto CPU::readDouble(u32 address) -> maybe<u64> {
  return bus.readDouble(address);
}

auto CPU::readDouble(u32 address, u32 mask) -> maybe<u64> {
  return bus.readDouble(address);
}

auto CPU::writeByte(u32 address, u8 data) -> void {
  return bus.writeByte(address, data);
}

auto CPU::writeHalf(u32 address, u16 data) -> void {
  return bus.writeHalf(address, data);
}

auto CPU::writeWord(u32 address, u32 data) -> void {
  return bus.writeWord(address, data);
}

auto CPU::writeWord(u32 address, u32 mask, u32 data) -> void {
  return bus.writeWord(address, data);
}

auto CPU::writeDouble(u32 address, u64 data) -> void {
  return bus.writeDouble(address, data);
}

auto CPU::writeDouble(u32 address, u32 mask, u64 data) -> void {
  return bus.writeDouble(address, data);
}
