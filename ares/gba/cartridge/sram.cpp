auto Cartridge::SRAM::read(u32 mode, n32 address) -> n32 {
  n32 word = data[address & mask];
  word |= word <<  8;
  word |= word << 16;
  return word;
}

auto Cartridge::SRAM::write(u32 mode, n32 address, n32 word) -> void {
  data[address & mask] = word;
}
