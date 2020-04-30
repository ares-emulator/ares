inline auto Bus::readByte(uint32_t address) -> uint8_t {
  address &= 0x1fff'ffff;

  if(address <= 0x007f'ffff) {
    return rdram.read(address);
  }

  if(address <= 0x03ff'ffff) {
    return 0;
  }

  if(address <= 0x0400'0fff) {
    return rsp.dmem.read(address & 0xfff);
  }

  if(address <= 0x0400'1fff) {
    return rsp.imem.read(address & 0xfff);
  }

  if(address <= 0x0fff'ffff) {
    return 0;
  }

  if(address <= 0x17ff'ffff) {
    return cartridge.rom.read(address & cartridge.rom.mask);
  }

  return 0;
}

inline auto Bus::readHalf(uint32_t address) -> uint16_t {
  uint16_t data;
  data  = readByte(address + 0) << 8;
  data |= readByte(address + 1) << 0;
  return data;
}

inline auto Bus::readWord(uint32_t address) -> uint32_t {
  uint32_t data;
  data  = readHalf(address + 0) << 16;
  data |= readHalf(address + 2) <<  0;
  return data;
}

inline auto Bus::readQuad(uint32_t address) -> uint64_t {
  uint64_t data;
  data  = readWord(address + 0) << 32;
  data |= readWord(address + 4) <<  0;
  return data;
}

inline auto Bus::writeByte(uint32_t address, uint8_t data) -> void {
  address &= 0x1fff'ffff;

  if(address <= 0x007f'ffff) {
    return rdram.write(address, data);
  }

  if(address <= 0x03ff'ffff) {
    return;
  }

  if(address <= 0x0400'0fff) {
    return rsp.dmem.write(address & 0xfff, data);
  }

  if(address <= 0x0400'1fff) {
    return rsp.imem.write(address & 0xfff, data);
  }

  if(address <= 0x0fff'ffff) {
    return;
  }
}

inline auto Bus::writeHalf(uint32_t address, uint16_t data) -> void {
  writeByte(address + 0, data >> 8);
  writeByte(address + 1, data >> 0);
}

inline auto Bus::writeWord(uint32_t address, uint32_t data) -> void {
  writeHalf(address + 0, data >> 16);
  writeHalf(address + 2, data >>  0);
}

inline auto Bus::writeQuad(uint32_t address, uint64_t data) -> void {
  writeWord(address + 0, data >> 32);
  writeWord(address + 4, data >>  0);
}
