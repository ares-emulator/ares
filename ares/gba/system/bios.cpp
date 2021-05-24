auto BIOS::load(Node::Object parent) -> void {
  rom.allocate(16_KiB);
  if(auto fp = system.pak->read("bios.rom")) {
    rom.load(fp);
  }
}

auto BIOS::unload() -> void {
  rom.reset();
}

auto BIOS::read(u32 mode, n25 address) -> n32 {
  //unmapped memory
  if(address >= 0x0000'4000) {
    if(cpu.context.dmaActive) return cpu.dmabus.data;
    return cpu.pipeline.fetch.instruction;  //0000'4000-01ff'ffff
  }

  //GBA BIOS is read-protected; only the BIOS itself can read its own memory
  //when accessed elsewhere; this should return the last value read by the BIOS program
  if(!cpu.memory.biosSwap) {
    if(cpu.processor.r15 >= 0x0000'4000) return mdr;
  } else {
    if(cpu.processor.r15 <= 0x01ff'ffff) return mdr;
    if(cpu.processor.r15 >= 0x0200'4000) return mdr;
  }

  if(mode & Word) return mdr = read(Half, address &~ 2) << 0 | read(Half, address | 2) << 16;
  if(mode & Half) return mdr = read(Byte, address &~ 1) << 0 | read(Byte, address | 1) <<  8;

  return mdr = rom.read(address);
}

auto BIOS::write(u32 mode, n25 address, n32 word) -> void {
}

auto BIOS::serialize(serializer& s) -> void {
  s(mdr);
}
