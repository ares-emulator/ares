auto Linear::load(Markup::Node document) -> void {
  if(auto memory = document["game/board/memory(type=ROM,content=Program)"]) {
    rom.allocate(memory["size"].natural());
    if(auto fp = platform->open(cartridge.node, "program.rom", File::Read, File::Required)) {
      rom.load(fp);
    }
  }
}

auto Linear::save(Markup::Node document) -> void {
}

auto Linear::read(uint8 bank, uint13 address) -> uint8 {
  return rom.read((uint7)bank << 13 | address);
}

auto Linear::write(uint8 bank, uint13 address, uint8 data) -> void {
}

auto Linear::power() -> void {
}

auto Linear::serialize(serializer& s) -> void {
}
