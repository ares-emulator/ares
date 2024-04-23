auto Cartridge::SRAM::read(n32 address) -> n8 {
  return data[address & mask];
}

auto Cartridge::SRAM::write(n32 address, n32 word) -> void {
  data[address & mask] = word;
}
