auto CPU::readByte(u32 address) -> maybe<u32> {
  return {};
}

auto CPU::readHalf(u32 address) -> maybe<u32> {
  return {};
}

auto CPU::readWord(u32 address) -> maybe<u32> {
  return {};
}

auto CPU::readWord(u32 address, u32 mask) -> maybe<u32> {
  return {};
}

auto CPU::readDouble(u32 address) -> maybe<u64> {
  return {};
}

auto CPU::readDouble(u32 address, u32 mask) -> maybe<u64> {
  return {};
}

auto CPU::writeByte(u32 address, u8 data) -> void {
}

auto CPU::writeHalf(u32 address, u16 data) -> void {
}

auto CPU::writeWord(u32 address, u32 data) -> void {
}

auto CPU::writeWord(u32 address, u32 mask, u32 data) -> void {
}

auto CPU::writeDouble(u32 address, u64 data) -> void {
}

auto CPU::writeDouble(u32 address, u32 mask, u64 data) -> void {
}
