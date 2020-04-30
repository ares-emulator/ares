auto CPU::readByte(uint32_t address) -> maybe<uint32_t> {
  return {};
}

auto CPU::readHalf(uint32_t address) -> maybe<uint32_t> {
  return {};
}

auto CPU::readWord(uint32_t address) -> maybe<uint32_t> {
  return {};
}

auto CPU::readWord(uint32_t address, uint32_t mask) -> maybe<uint32_t> {
  return {};
}

auto CPU::readDouble(uint32_t address) -> maybe<uint64_t> {
  return {};
}

auto CPU::readDouble(uint32_t address, uint32_t mask) -> maybe<uint64_t> {
  return {};
}

auto CPU::writeByte(uint32_t address, uint8_t data) -> void {
}

auto CPU::writeHalf(uint32_t address, uint16_t data) -> void {
}

auto CPU::writeWord(uint32_t address, uint32_t data) -> void {
}

auto CPU::writeWord(uint32_t address, uint32_t mask, uint32_t data) -> void {
}

auto CPU::writeDouble(uint32_t address, uint64_t data) -> void {
}

auto CPU::writeDouble(uint32_t address, uint32_t mask, uint64_t data) -> void {
}
