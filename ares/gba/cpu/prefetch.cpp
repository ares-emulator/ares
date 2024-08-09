auto CPU::prefetchSync(n32 address) -> void {
  if(address == prefetch.addr) return;

  if(prefetch.wait == 1) step(1);

  prefetch.addr = address;
  prefetch.load = address;
  prefetch.wait = _wait(Half | Nonsequential, prefetch.load);
}

auto CPU::prefetchStep(u32 clocks) -> void {
  step(clocks);
  if(!wait.prefetch || context.dmaRomAccess || (prefetch.addr == 0)) return;

  while(!prefetch.full() && clocks--) {
    if(--prefetch.wait) continue;
    prefetch.slot[prefetch.load >> 1 & 7] = cartridge.read(Half, prefetch.load);
    prefetch.load += 2;
    prefetch.wait = _wait(Half | Sequential, prefetch.load);
  }
}

auto CPU::prefetchReset() -> void {
  if(prefetch.wait == 1) step(1);

  prefetch.addr = 0;
  prefetch.load = 0;
  prefetch.wait = 0;
}

auto CPU::prefetchRead(u32 mode) -> n32 {
  //run prefetcher for 1 cycle or until buffer contains an instruction, blocking DMA
  context.prefetchActive = 1;
  if(prefetch.empty() && (mode & Word)) {
    prefetchStep(prefetch.wait + _wait(Half | Sequential, prefetch.load + 2));
  } else if(prefetch.empty() && (mode & Half)) {
    prefetchStep(prefetch.wait);
  } else if((prefetch.size() == 1) && (mode & Word)) {
    prefetchStep(prefetch.wait);
  } else {
    prefetchStep(1);
  }
  context.prefetchActive = 0;

  if(prefetch.full()) prefetch.wait = _wait(Half | Sequential, prefetch.load);

  n32 word = prefetch.slot[prefetch.addr >> 1 & 7];
  prefetch.addr += 2;
  if(mode & Word) {
    word |= prefetch.slot[prefetch.addr >> 1 & 7] << 16;
    prefetch.addr += 2;
  }
  return word;
}
