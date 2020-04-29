auto VR4300::readByte(uint32_t address) -> maybe<uint32_t> {
  return {};
}

auto VR4300::readHalf(uint32_t address) -> maybe<uint32_t> {
  return {};
}

auto VR4300::readWord(uint32_t address) -> maybe<uint32_t> {
  return {};
}

auto VR4300::readWord(uint32_t address, uint32_t mask) -> maybe<uint32_t> {
  return {};
}

auto VR4300::readDouble(uint32_t address) -> maybe<uint64_t> {
  return {};
}

auto VR4300::readDouble(uint32_t address, uint32_t mask) -> maybe<uint64_t> {
  return {};
}

auto VR4300::writeByte(uint32_t address, uint8_t data) -> void {
}

auto VR4300::writeHalf(uint32_t address, uint16_t data) -> void {
}

auto VR4300::writeWord(uint32_t address, uint32_t data) -> void {
}

auto VR4300::writeWord(uint32_t address, uint32_t mask, uint32_t data) -> void {
}

auto VR4300::writeDouble(uint32_t address, uint64_t data) -> void {
}

auto VR4300::writeDouble(uint32_t address, uint32_t mask, uint64_t data) -> void {
}
