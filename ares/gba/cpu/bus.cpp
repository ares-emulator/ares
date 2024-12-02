auto CPU::sleep() -> void {
  dmaRun();
  ARM7TDMI::irq = irq.synchronizer;
  prefetchStep(1);
}

template <bool UseDebugger>
inline auto CPU::getBus(u32 mode, n32 address) -> n32 {
  u32 clocks = _wait(mode, address);
  u32 word;

  if(address >= 0x1000'0000) {
    if constexpr(!UseDebugger) prefetchStep(clocks);
    return openBus.get(mode, address);
  } else if(address & 0x0800'0000) {
    if(mode & Prefetch && wait.prefetch) {
      prefetchSync(address);
      prefetchStep(1);
      word = prefetchRead();
      if(mode & Word) word |= prefetchRead() << 16;
    } else {
      if constexpr(!UseDebugger) prefetchReset();
      if constexpr(!UseDebugger) step(clocks);
      word = cartridge.read(mode, address);
    }
  } else {
    if(memory.biosSwap && address < 0x0400'0000) address ^= 0x0200'0000;
    if constexpr(!UseDebugger) prefetchStep(clocks);
    if(auto result = platform->cheat(address)) return *result;
         if(address <  0x0200'0000) word = bios.read(mode, address);
    else if(address <  0x0300'0000) word = readEWRAM(mode, address);
    else if(address <  0x0400'0000) word = readIWRAM(mode, address);
    else if(address >= 0x0700'0000) word = ppu.readOAM(mode, address);
    else if(address >= 0x0600'0000) word = ppu.readVRAM(mode, address);
    else if(address >= 0x0500'0000) word = ppu.readPRAM(mode, address);
    else if((address & 0xffff'fc00) == 0x0400'0000) word = bus.io[address & 0x3ff]->readIO(mode, address);
    else if((address & 0xff00'ffff) == 0x0400'0800) word = ((IO*)this)->readIO(mode, 0x0400'0800 | (address & 3));
    else return openBus.get(mode, address);
  }

  openBus.set(mode, address, word);

  return word;
}

auto CPU::get(u32 mode, n32 address) -> n32 {
  dmaRun();
  ARM7TDMI::irq = irq.synchronizer;
  return getBus<false>(mode, address);
}

auto CPU::getDebugger(u32 mode, n32 address) -> n32 {
  return getBus<true>(mode, address);
}

auto CPU::set(u32 mode, n32 address, n32 word) -> void {
  dmaRun();
  ARM7TDMI::irq = irq.synchronizer;
  u32 clocks = _wait(mode, address);

  if(address >= 0x1000'0000) {
    prefetchStep(clocks);
  } else if(address & 0x0800'0000) {
    prefetchReset();
    step(clocks);
    cartridge.write(mode, address, word);
  } else {
    if(memory.biosSwap && address < 0x0400'0000) address ^= 0x0200'0000;
    prefetchStep(clocks);
         if(address  < 0x0200'0000) bios.write(mode, address, word);
    else if(address  < 0x0300'0000) writeEWRAM(mode, address, word);
    else if(address  < 0x0400'0000) writeIWRAM(mode, address, word);
    else if(address >= 0x0700'0000) ppu.writeOAM(mode, address, word);
    else if(address >= 0x0600'0000) ppu.writeVRAM(mode, address, word);
    else if(address >= 0x0500'0000) ppu.writePRAM(mode, address, word);
    else if((address & 0xffff'fc00) == 0x0400'0000) bus.io[address & 0x3ff]->writeIO(mode, address, word);
    else if((address & 0xff00'ffff) == 0x0400'0800) ((IO*)this)->writeIO(mode, 0x0400'0800 | (address & 3), word);
  }

  openBus.set(mode, address, word);
}

auto CPU::_wait(u32 mode, n32 address) -> u32 {
  if(address >= 0x1000'0000) return 1;  //unmapped
  if(address <  0x0200'0000) return 1;
  if(address <  0x0300'0000) return (16 - memory.ewramWait) * (mode & Word ? 2 : 1);
  if(address <  0x0500'0000) return 1;
  if(address <  0x0700'0000) return mode & Word ? 2 : 1;
  if(address <  0x0800'0000) return 1;

  static u32 timings[] = {5, 4, 3, 9};
  u32 n = timings[wait.nwait[address >> 25 & 3]];
  u32 s = wait.swait[address >> 25 & 3];

  switch(address & 0x0e00'0000) {
  case 0x0800'0000: s = s ? 2 : 3; break;
  case 0x0a00'0000: s = s ? 2 : 5; break;
  case 0x0c00'0000: s = s ? 2 : 9; break;
  case 0x0e00'0000: s = n; break;
  }

  bool sequential = (mode & Sequential);
  if((address & 0x1fffe) == 0) sequential = false;  //N cycle on 16-bit ROM crossing 128KB page boundary (RAM S==N)

  u32 clocks = sequential ? s : n;
  if(mode & Word) clocks += s;  //16-bit bus requires two transfers for words
  return clocks;
}

auto CPU::OpenBus::get(u32 mode, n32 address) -> n32 {
  if(mode & Word) address &= ~3;
  if(mode & Half) address &= ~1;
  return data >> (8 * (address & 3));
}

auto CPU::OpenBus::set(u32 mode, n32 address, n32 word) -> void {
  if(address >> 24 == 0x3) {
    //open bus from IWRAM only overwrites part of the last IWRAM value accessed
    if(mode & Word) {
      iwramData = word;
    } else if(mode & Half) {
      if(address & 2) {
        iwramData.bit(16,31) = (n16)word;
      } else {
        iwramData.bit( 0,15) = (n16)word;
      }
    } else if(mode & Byte) {
      iwramData.byte(address & 3) = (n8)word;
    }
    data = iwramData;
  } else {
    if(mode & Byte) word = (word & 0xff) * 0x01010101;
    if(mode & Half) word = (word & 0xffff) * 0x00010001;
    data = word;
  }
}
