struct Unmapped : Interface {
  auto readByte(u32 address) -> u32 {
    debug(unhandled, "Unmapped::readByte(", hex(address, 8L), ")");
    return 0;
  }

  auto readHalf(u32 address) -> u32 {
    debug(unhandled, "Unmapped::readHalf(", hex(address, 8L), ")");
    return 0;
  }

  auto readWord(u32 address) -> u32 {
    debug(unhandled, "Unmapped::readWord(", hex(address, 8L), ")");
    return 0;
  }

  auto writeByte(u32 address, u32 data) -> void {
    debug(unhandled, "Unmapped::writeByte(", hex(address, 8L), ")");
  }

  auto writeHalf(u32 address, u32 data) -> void {
    debug(unhandled, "Unmapped::writeHalf(", hex(address, 8L), ")");
  }

  auto writeWord(u32 address, u32 data) -> void {
    debug(unhandled, "Unmapped::writeWord(", hex(address, 8L), ")");
  }
};
