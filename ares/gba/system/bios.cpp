auto BIOS::load(Node::Object parent) -> void {
  rom.allocate(16_KiB);
  if(auto fp = system.pak->read("bios.rom")) {
    rom.load(fp);
  }
}

auto BIOS::unload() -> void {
  rom.reset();
}

auto BIOS::readROM(n25 address) -> n32 {
  address &= ~3;
  return mdr = rom.read(address) << 0 | rom.read(address + 1) << 8 | rom.read(address + 2) << 16 | rom.read(address + 3) << 24;
}

auto BIOS::read(u32 mode, n25 address) -> n32 {
  //unmapped memory
  if(address >= 0x0000'4000) {
    return cpu.pipeline.fetch.instruction;  //0000'4000-01ff'ffff
  }

  //GBA BIOS is read-protected; only the BIOS itself can read its own memory
  //when accessed elsewhere; this should return the last value read by the BIOS program
  if(!cpu.memory.biosSwap) {
    if(cpu.processor.r15 < 0x0000'4000) mdr = readROM(address);
  } else {
    if(cpu.processor.r15 > 0x01ff'ffff && cpu.processor.r15 < 0x0200'4000) mdr = readROM(address);
  }

  if(mode & Word) address &= ~3;
  if(mode & Half) address &= ~1;
  return mdr >> (8 * (address & 3));
}

auto BIOS::write(u32 mode, n25 address, n32 word) -> void {
}

auto BIOS::serialize(serializer& s) -> void {
  s(mdr);
}
