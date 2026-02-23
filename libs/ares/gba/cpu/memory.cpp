template <bool UseDebugger>
auto CPU::readIWRAM(u32 mode, n32 address) -> n32 {
  if constexpr(!UseDebugger) prefetchStep(1);
  n32 word;
  if(mode & Word) {
    address &= 0x7ffc;
    word = iwram[address + 0] << 0 | iwram[address + 1] << 8 | iwram[address + 2] << 16 | iwram[address + 3] << 24;
  } else if(mode & Half) {
    address &= 0x7ffe;
    word = iwram[address + 0] << 0 | iwram[address + 1] << 8;
  } else {
    address &= 0x7fff;
    word = iwram[address];
  }

  return word;
}

auto CPU::writeIWRAM(u32 mode, n32 address, n32 word) -> void {
  prefetchStep(1);
  if(mode & Word) {
    address &= 0x7ffc;
    iwram[address + 0] = word >>  0;
    iwram[address + 1] = word >>  8;
    iwram[address + 2] = word >> 16;
    iwram[address + 3] = word >> 24;
    return;
  } else if(mode & Half) {
    address &= 0x7ffe;
    iwram[address + 0] = word >> 0;
    iwram[address + 1] = word >> 8;
  } else {
    address &= 0x7fff;
    iwram[address] = word;
  }
}

template <bool UseDebugger>
auto CPU::readEWRAM(u32 mode, n32 address) -> n32 {
  if(!memory.ewram) return readIWRAM<UseDebugger>(mode, address);
  if(mode & Word) return readEWRAM<UseDebugger>(Half, address &~ 2) << 0 | readEWRAM<UseDebugger>(Half, address | 2) << 16;

  if constexpr(!UseDebugger) prefetchStep(16 - memory.ewramWait);
  n16 half;
  address &= 0x3ffff;
  if(mode & Half) {
    half = ewram[address & ~1] << 0 | ewram[address | 1] << 8;
  } else {
    half = ewram[address];
  }

  return half;
}

auto CPU::writeEWRAM(u32 mode, n32 address, n32 word) -> void {
  if(!memory.ewram) return writeIWRAM(mode, address, word);
  if(mode & Word) {
    writeEWRAM(Half, address &~2, word >>  0);
    writeEWRAM(Half, address | 2, word >> 16);
    return;
  }

  prefetchStep(16 - memory.ewramWait);
  address &= 0x3ffff;
  if(mode & Half) {
    ewram[address & ~1] = word >> 0;
    ewram[address |  1] = word >> 8;
  } else {
    ewram[address & 0x3ffff] = word;
  }
}

template <bool UseDebugger>
auto CPU::readPRAM(u32 mode, n32 address) -> n32 {
  if(mode & Word) return readPRAM<UseDebugger>(Half, address & ~2) << 0 | readPRAM<UseDebugger>(Half, address | 2) << 16;

  //stall until PPU is no longer accessing PRAM (minimum 1 cycle)
  if constexpr(!UseDebugger) {
    do {
      prefetchStep(1);
      synchronize(ppu);
    } while(ppu.pramContention());
  }

  return ppu.readPRAM(mode, address);
}

auto CPU::writePRAM(u32 mode, n32 address, n32 word) -> void {
  if(mode & Word) {
    writePRAM(Half, address & ~2, word >>  0);
    writePRAM(Half, address |  2, word >> 16);
    return;
  }

  //stall until PPU is no longer accessing PRAM (minimum 1 cycle)
  do {
    prefetchStep(1);
    synchronize(ppu);
  } while(ppu.pramContention());

  ppu.writePRAM(mode, address, word);
}

template <bool UseDebugger>
auto CPU::readVRAM(u32 mode, n32 address) -> n32 {
  if(mode & Word) return readVRAM<UseDebugger>(Half, address & ~2) << 0 | readVRAM<UseDebugger>(Half, address | 2) << 16;

  //stall until PPU is no longer accessing VRAM (minimum 1 cycle)
  if constexpr(!UseDebugger) {
    do {
      prefetchStep(1);
      synchronize(ppu);
    } while(ppu.vramContention(address));
  }

  return ppu.readVRAM(mode, address);
}

auto CPU::writeVRAM(u32 mode, n32 address, n32 word) -> void {
  if(mode & Word) {
    writeVRAM(Half, address & ~2, word >>  0);
    writeVRAM(Half, address |  2, word >> 16);
    return;
  }

  //stall until PPU is no longer accessing VRAM (minimum 1 cycle)
  do {
    prefetchStep(1);
    synchronize(ppu);
  } while(ppu.vramContention(address));

  ppu.writeVRAM(mode, address, word);
}

template<bool UseDebugger>
auto CPU::readROM(u32 mode, n32 address) -> n32 {
  if(mode & Word) {
    n32 word = 0;
    word |= readROM<UseDebugger>(Half, address & ~2) <<  0;
    word |= readROM<UseDebugger>(Half, address |  2) << 16;
    return word;
  }

  if constexpr(!UseDebugger) {
    if(!context.burstActive) cartridge.startBurst(address);
    step(waitCartridge(address, context.burstActive));
  }
  n16 half = cartridge.readRom<UseDebugger>(address);
  if constexpr(!UseDebugger) context.burstActive = true;

  if(mode & Byte) return half.byte(address & 1);
  return half;
}

auto CPU::writeROM(u32 mode, n32 address, n32 word) -> void {
  if(mode & Word) {
    writeROM(Half, address & ~2, word >>  0);
    writeROM(Half, address |  2, word >> 16);
    return;
  }

  if(!context.burstActive) cartridge.startBurst(address);
  step(waitCartridge(address, context.burstActive));
  cartridge.writeRom(address, word);
  context.burstActive = true;
  return;
}
