template <bool UseDebugger>
auto CPU::readIWRAM(u32 mode, n32 address) -> n32 {
  if constexpr(UseDebugger) {
    address &= 0x7ffc;
    return iwram[address + 0] << 0 | iwram[address + 1] << 8 | iwram[address + 2] << 16 | iwram[address + 3] << 24;
  }
  prefetchStep(1);
  if(mode & Word) {
    address &= 0x7ffc;
    iwramBus = iwram[address + 0] << 0 | iwram[address + 1] << 8 | iwram[address + 2] << 16 | iwram[address + 3] << 24;
  } else if(mode & Half) {
    address &= 0x7ffe;
    n16 half = iwram[address + 0] << 0 | iwram[address + 1] << 8;
    if(address & 2) {
      iwramBus.bit(16,31) = half;
    } else {
      iwramBus.bit( 0,15) = half;
    }
  } else {
    address &= 0x7fff;
    iwramBus.byte(address & 3) = iwram[address];
  }
  return iwramBus;
}

auto CPU::writeIWRAM(u32 mode, n32 address, n32 word) -> void {
  prefetchStep(1);
  if(mode & Word) {
    address &= 0x7ffc;
    iwramBus = word;
    iwram[address + 0] = word >>  0;
    iwram[address + 1] = word >>  8;
    iwram[address + 2] = word >> 16;
    iwram[address + 3] = word >> 24;
    return;
  } else if(mode & Half) {
    address &= 0x7ffe;
    if(address & 2) {
      iwramBus.bit(16,31) = word;
    } else {
      iwramBus.bit( 0,15) = word;
    }
    iwram[address + 0] = word >> 0;
    iwram[address + 1] = word >> 8;
  } else {
    address &= 0x7fff;
    iwramBus.byte(address & 3) = word;
    iwram[address] = word;
  }
}

template <bool UseDebugger>
auto CPU::readEWRAM(u32 mode, n32 address) -> n32 {
  if(mode & Word) return (n16)readEWRAM<UseDebugger>(Half, address & ~2) << 0 | (n16)readEWRAM<UseDebugger>(Half, address | 2) << 16;

  if constexpr(!UseDebugger) prefetchStep(16 - memory.ewramWait);
  address &= 0x3ffff;
  n16 half = ewram[address & ~1] << 0 | ewram[address | 1] << 8;
  return half << 0 | half << 16;
}

auto CPU::writeEWRAM(u32 mode, n32 address, n32 word) -> void {
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
  if(mode & Word) return (n16)readPRAM<UseDebugger>(Half, address & ~2) << 0 | (n16)readPRAM<UseDebugger>(Half, address | 2) << 16;

  //stall until PPU is no longer accessing PRAM (minimum 1 cycle)
  if constexpr(!UseDebugger) {
    do {
      prefetchStep(1);
      synchronize(ppu);
    } while(ppu.pramContention());
  }

  n16 half = ppu.readPRAM(mode, address);
  return half << 0 | half << 16;
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
  if(mode & Word) return (n16)readVRAM<UseDebugger>(Half, address & ~2) << 0 | (n16)readVRAM<UseDebugger>(Half, address | 2) << 16;

  //stall until PPU is no longer accessing VRAM (minimum 1 cycle)
  if constexpr(!UseDebugger) {
    do {
      prefetchStep(1);
      synchronize(ppu);
    } while(ppu.vramContention(address));
  }

  n16 half = ppu.readVRAM(mode, address);
  return half << 0 | half << 16;
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
    word |= (n16)readROM<UseDebugger>(Half, address & ~2) <<  0;
    word |= (n16)readROM<UseDebugger>(Half, address |  2) << 16;
    return word;
  }

  if constexpr(!UseDebugger) {
    if(!context.burstActive) cartridge.startBurst(address);
    step(waitCartridge(address, context.burstActive));
  }
  n16 half = cartridge.readRom<UseDebugger>(address);
  if constexpr(!UseDebugger) context.burstActive = true;
  return half << 0 | half << 16;
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
