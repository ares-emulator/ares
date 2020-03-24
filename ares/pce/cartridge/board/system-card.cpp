auto SystemCard::load(Markup::Node document) -> void {
  if(auto memory = document["game/board/memory(type=ROM,content=Program)"]) {
    rom.allocate(memory["size"].natural());
    if(auto fp = platform->open(cartridge.node, "program.rom", File::Read, File::Required)) {
      rom.load(fp);
    }
  }

  if(auto memory = document["game/board/memory(type=RAM,content=Work)"]) {
    ram.allocate(memory["size"].natural());
  }
}

auto SystemCard::save(Markup::Node document) -> void {
}

auto SystemCard::read(uint8 bank, uint13 address) -> uint8 {
  uint8 data = 0xff;

  if(bank >= 0x00 && bank <= 0x3f) {
    return rom.read((uint6)bank << 13 | address);
  }

  if(bank >= 0x68 && bank <= 0x7f) {
    if(!ram) return data;
    return ram.read((bank - 0x68) << 13 | address);
  }

  return data;
}

auto SystemCard::write(uint8 bank, uint13 address, uint8 data) -> void {
  if(bank >= 0x68 && bank <= 0x7f) {
    if(!ram) return;
    return ram.write((bank - 0x68) << 13 | address, data);
  }
}

auto SystemCard::power() -> void {
}

auto SystemCard::serialize(serializer& s) -> void {
  if(ram) ram.serialize(s);
}
