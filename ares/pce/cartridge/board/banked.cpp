auto Banked::load(Markup::Node document) -> void {
  if(auto memory = document["game/board/memory(type=ROM,content=Program)"]) {
    rom.allocate(memory["size"].natural());
    if(auto fp = platform->open(cartridge.node, "program.rom", File::Read, File::Required)) {
      rom.load(fp);
    }
  }
}

auto Banked::save(Markup::Node document) -> void {
}

auto Banked::read(uint8 bank, uint13 address) -> uint8 {
  if(bank >= 0x00 && bank <= 0x3f) {
    return rom.read((uint6)bank << 13 | address);
  }

  if(bank >= 0x40 && bank <= 0x7f) {
    return rom.read(1 + romBank << 19 | (uint6)bank << 13 | address);
  }

  return 0x00;
}

auto Banked::write(uint8 bank, uint13 address, uint8 data) -> void {
  if(bank == 0x00 && address >= 0x01ff0 && address <= 0x01ffe) {
    romBank = address.bit(0,1);
  }
}

auto Banked::power() -> void {
  romBank = 0;
}

auto Banked::serialize(serializer& s) -> void {
  s.integer(romBank);
}
