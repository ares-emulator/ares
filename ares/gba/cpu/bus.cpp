auto CPU::sleep() -> void {
  if(!dmac.step()) {
    ARM7TDMI::irq = irq.synchronizer;
    prefetchStep(1);
  }
}

template <bool IsDMA, bool UseDebugger>
inline auto CPU::getBus(u32 mode, n32 address) -> n32 {
  if constexpr(!IsDMA) dmac.runPending();
  if constexpr(!UseDebugger) {
    ARM7TDMI::irq = irq.synchronizer;
    context.romAccess = false;
  }
  u32 word;

  if(memory.biosSwap && address < 0x0400'0000) address ^= 0x0200'0000;

  switch(address >> 24) {

  case 0x00: case 0x01:
    if constexpr(!UseDebugger) prefetchStep(1);
    word = bios.read(mode, address);
    break;

  case 0x02: word = readEWRAM<UseDebugger>(mode, address); break;
  case 0x03: word = readIWRAM<UseDebugger>(mode, address); break;

  case 0x04:
    if constexpr(!UseDebugger) prefetchStep(1);
         if((address & 0xffff'fc00) == 0x0400'0000) word = bus.io[address & 0x3ff]->readIO(mode, address);
    else if((address & 0xff00'fffc) == 0x0400'0800) word = ((IO*)this)->readIO(mode, 0x0400'0800 | (address & 3));
    else return openBus.get(mode, address);
    break;

  case 0x05: word = readPRAM<UseDebugger>(mode, address); break;
  case 0x06: word = readVRAM<UseDebugger>(mode, address); break;

  case 0x07:
    if constexpr(!UseDebugger) prefetchStep(1);
    word = ppu.readOAM(mode, address);
    break;

  case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d:
    if constexpr(UseDebugger) return readROM<true>(mode, address);
    context.burstActive = checkBurst<IsDMA>(mode);
    context.romAccess = true;
    if(mode & Prefetch) {
      if((address & 0x1fffe) && wait.prefetch && address == prefetch.addr && (!prefetch.empty() || prefetch.ahead)) {
        prefetchStep(1);
        word = prefetchRead();
        if(mode & Word) word |= prefetchRead() << 16;
      } else {
        if(mode & Word) address &= ~3;  //prevents misaligned PC from reading incorrect values
        prefetchSync(mode, address);
        word = readROM<false>(mode, address);
      }
    } else {
      if constexpr(IsDMA) dmac.romBurst = true;
      prefetchReset();
      word = readROM<false>(mode, address);
    }
    break;

  case 0x0e: case 0x0f:
    if constexpr(!UseDebugger) prefetchReset();
    if constexpr(!UseDebugger) step(waitCartridge(address, false));
    word = cartridge.readBackup(address) * 0x01010101;
    break;

  default:
    if constexpr(!UseDebugger) prefetchStep(1);
    return openBus.get(mode, address);

  }

  openBus.set(mode, address, word);

  if(auto result = platform->cheat(address)) return *result;
  return word;
}

auto CPU::get(u32 mode, n32 address) -> n32 {
  u32 word = getBus<false, false>(mode, address);
  if(!context.romAccess) cartridge.mrom.burst = false;
  return word;
}

auto CPU::getDMA(u32 mode, n32 address) -> n32 {
  return getBus<true, false>(mode, address);
}

auto CPU::getDebugger(u32 mode, n32 address) -> n32 {
  return getBus<false, true>(mode, address);
}

template <bool IsDMA>
auto CPU::setBus(u32 mode, n32 address, n32 word) -> void {
  if constexpr(!IsDMA) dmac.runPending();
  ARM7TDMI::irq = irq.synchronizer;
  context.romAccess = false;

  if(memory.biosSwap && address < 0x0400'0000) address ^= 0x0200'0000;

  switch(address >> 24) {

  case 0x00: case 0x01:
    prefetchStep(1);
    bios.write(mode, address, word);
    break;

  case 0x02: writeEWRAM(mode, address, word); break;
  case 0x03: writeIWRAM(mode, address, word); break;

  case 0x04:
    prefetchStep(1);
         if((address & 0xffff'fc00) == 0x0400'0000) bus.io[address & 0x3ff]->writeIO(mode, address, word);
    else if((address & 0xff00'fffc) == 0x0400'0800) ((IO*)this)->writeIO(mode, 0x0400'0800 | (address & 3), word);
    break;

  case 0x05: writePRAM(mode, address, word); break;
  case 0x06: writeVRAM(mode, address, word); break;

  case 0x07:
    prefetchStep(1);
    synchronize(ppu);
    ppu.writeOAM(mode, address, word);
    break;

  case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d:
    context.burstActive = checkBurst<IsDMA>(mode);
    context.romAccess = true;
    if constexpr(IsDMA) dmac.romBurst = true;
    prefetchReset();
    writeROM(mode, address, word);
    break;

  case 0x0e: case 0x0f:
    prefetchReset();
    step(waitCartridge(address, false));
    if(mode & Word) word >>= 8 * (address & 3);
    if(mode & Half) word >>= 8 * (address & 1);
    cartridge.writeBackup(address, word);
    break;

  default:
    prefetchStep(1);
    break;

  }

  openBus.set(mode, address, word);
  if constexpr(!IsDMA) {
    if(!context.romAccess) cartridge.mrom.burst = false;
  }
}

auto CPU::set(u32 mode, n32 address, n32 word) -> void {
  setBus<false>(mode, address, word);
}

auto CPU::setDMA(u32 mode, n32 address, n32 word) -> void {
  setBus<true>(mode, address, word);
}

auto CPU::lock() -> void {
  dmac.runPending();
  context.busLocked = true;
}

auto CPU::unlock() -> void {
  context.busLocked = false;
}

auto CPU::waitCartridge(n32 address, bool sequential) -> u32 {
  static u32 timings[] = {5, 4, 3, 9};
  u32 n = timings[wait.nwait[address >> 25 & 3]];
  u32 s = wait.swait[address >> 25 & 3];

  switch(address & 0x0e00'0000) {
  case 0x0800'0000: s = s ? 2 : 3; break;
  case 0x0a00'0000: s = s ? 2 : 5; break;
  case 0x0c00'0000: s = s ? 2 : 9; break;
  case 0x0e00'0000: s = n; break;
  }

  u32 clocks = sequential ? s : n;
  return clocks;
}

template <bool IsDMA>
auto CPU::checkBurst(u32 mode) -> bool {
  //check whether burst transfer is in progress
  if(cartridge.mrom.burst == false) return false;
  if constexpr(IsDMA) return dmac.romBurst;
  return !ARM7TDMI::nonsequential;
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
