auto RAM::load(Markup::Node document) -> void {
  if(auto memory = document["game/board/memory(type=ROM,content=Program)"]) {
    rom.allocate(memory["size"].natural());
    if(auto fp = platform->open(cartridge.node, "program.rom", File::Read, File::Required)) {
      rom.load(fp);
    }
  }

  if(auto memory = document["game/board/memory(type=RAM,content=Save)"]) {
    ram.allocate(memory["size"].natural());
    if(auto fp = platform->open(cartridge.node, "save.ram", File::Read)) {
      ram.load(fp);
    }
  }
}

auto RAM::save(Markup::Node document) -> void {
  if(auto memory = document["game/board/memory(type=RAM,content=Save)"]) {
    if(auto fp = platform->open(cartridge.node, "save.ram", File::Write)) {
      ram.save(fp);
    }
  }
}

auto RAM::read(uint8 bank, uint13 address) -> uint8 {
  return rom.read(bank << 13 | address);
}

auto RAM::write(uint8 bank, uint13 address, uint8 data) -> void {
}

auto RAM::power() -> void {
}

auto RAM::serialize(serializer& s) -> void {
  if(ram) ram.serialize(s);
}
