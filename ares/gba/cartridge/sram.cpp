auto Cartridge::SRAM::read(uint mode, uint32 address) -> uint32 {
  uint32 word = data[address & mask];
  word |= word <<  8;
  word |= word << 16;
  return word;
}

auto Cartridge::SRAM::write(uint mode, uint32 address, uint32 word) -> void {
  data[address & mask] = word;
}
