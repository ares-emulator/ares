auto CPU::prefetchSync(n32 address) -> void {
  if(address == prefetch.addr) return;

  if(prefetch.wait == 1) step(1);

  prefetch.stopped = false;
  prefetch.addr = address;
  prefetch.load = address;
  prefetch.wait = waitCartridge(Half | Nonsequential, prefetch.load);
}

auto CPU::prefetchStep(u32 clocks) -> void {
  step(clocks);
  if(!wait.prefetch || prefetch.stopped) return;

  while(!prefetch.full() && clocks--) {
    if(--prefetch.wait) continue;
    prefetch.slot[prefetch.load >> 1 & 7] = cartridge.readRom(Half, prefetch.load);
    prefetch.load += 2;
    prefetch.wait = waitCartridge(Half | Sequential, prefetch.load);
  }

  if(prefetch.full()) prefetch.stopped = true;
}

auto CPU::prefetchReset() -> void {
  if(prefetch.wait == 1) step(1);

  prefetch.stopped = true;
  prefetch.wait = 0;
  prefetch.addr = 0;
  prefetch.load = 0;
}

auto CPU::prefetchRead() -> n16 {
  if(prefetch.empty()) prefetchStep(prefetch.wait);

  n16 word = prefetch.slot[prefetch.addr >> 1 & 7];
  prefetch.addr += 2;

  if(prefetch.stopped && prefetch.empty()) {
    prefetch.stopped = false;
    prefetch.wait = waitCartridge(Half | Nonsequential, prefetch.load);
  }

  return word;
}
