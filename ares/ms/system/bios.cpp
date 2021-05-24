auto BIOS::load(Node::Object parent) -> void {
  if(auto fp = system.pak->read("bios.rom")) {
    rom.allocate(fp->size());
    rom.load(fp);
  }
}

auto BIOS::unload() -> void {
  rom.reset();
}

auto BIOS::read(n16 address, n8 data) -> n8 {
  if(!rom) return data;

  switch(address) {
  case 0x0000 ... 0x03ff: return rom.read(address);
  case 0x0400 ... 0x3fff: return rom.read(romBank[0] << 14 | (n14)address);
  case 0x4000 ... 0x7fff: return rom.read(romBank[1] << 14 | (n14)address);
  case 0x8000 ... 0xbfff: return rom.read(romBank[2] << 14 | (n14)address);
  }
  return data;
}

auto BIOS::write(n16 address, n8 data) -> void {
  if(!rom) return;

  switch(address) {
  case 0xfffd: romBank[0] = data; return;
  case 0xfffe: romBank[1] = data; return;
  case 0xffff: romBank[2] = data; return;
  }
}

auto BIOS::power() -> void {
  romBank[0] = 0;
  romBank[1] = 1;
  romBank[2] = 2;
}

auto BIOS::serialize(serializer& s) -> void {
  s(romBank);
}
