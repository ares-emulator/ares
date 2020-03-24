auto Cartridge::MBC0::read(uint16 address) -> uint8 {
  if((address & 0x8000) == 0x0000) {  //$0000-7fff
    return cartridge.rom.read((uint15)address);
  }

  if((address & 0xe000) == 0xa000) {  //$a000-bfff
    return cartridge.ram.read((uint13)address);
  }

  return 0xff;
}

auto Cartridge::MBC0::write(uint16 address, uint8 data) -> void {
  if((address & 0xe000) == 0xa000) {  //$a000-bfff
    cartridge.ram.write((uint13)address, data);
    return;
  }
}

auto Cartridge::MBC0::power() -> void {
}

auto Cartridge::MBC0::serialize(serializer& s) -> void {
}
