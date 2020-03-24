auto Split::load(Markup::Node document) -> void {
  if(auto memory = document["game/board/memory(type=ROM,content=Program)"]) {
    rom.allocate(memory["size"].natural());
    if(auto fp = platform->open(cartridge.node, "program.rom", File::Read, File::Required)) {
      rom.load(fp);
    }
  }

  //every licensed HuCard follows this format; but ideally the manifest should list two separate ROMs ...
  romAddress[0] = 0;
  romAddress[1] = bit::round(rom.size()) >> 1;
}

auto Split::save(Markup::Node document) -> void {
}

auto Split::read(uint8 bank, uint13 address) -> uint8 {
  uint19 linear = (uint6)bank << 13 | address;

  if(bank >= 0x00 && bank <= 0x3f) {
    return rom.read(romAddress[0] + linear);
  }

  if(bank >= 0x40 && bank <= 0x7f) {
    return rom.read(romAddress[1] + linear);
  }

  return 0x00;
}

auto Split::write(uint8 bank, uint13 address, uint8 data) -> void {
}

auto Split::power() -> void {
}

auto Split::serialize(serializer& s) -> void {
}
