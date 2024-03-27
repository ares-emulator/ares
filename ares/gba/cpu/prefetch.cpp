auto CPU::prefetchSync(n32 address) -> void {
  if(address == prefetch.addr) return;

  prefetch.addr = address;
  prefetch.load = address;
  prefetch.wait = _wait(Half | Nonsequential, prefetch.load);
}

auto CPU::prefetchStep(u32 clocks) -> void {
  step(clocks);
  if(!wait.prefetch || context.dmaActive) return;

  while(!prefetch.full() && clocks--) {
    if(--prefetch.wait) continue;
    prefetch.slot[prefetch.load >> 1 & 7] = cartridge.read(Half, prefetch.load);
    prefetch.load += 2;
    prefetch.wait = _wait(Half | Sequential, prefetch.load);
  }
}

auto CPU::prefetchReset() -> void {
  prefetch.addr = 0;
  prefetch.load = 0;
}

auto CPU::prefetchRead() -> n16 {
  if(prefetch.empty()) prefetchStep(prefetch.wait);

  if(prefetch.full()) prefetch.wait = _wait(Half | Sequential, prefetch.load);

  n16 half = prefetch.slot[prefetch.addr >> 1 & 7];
  prefetch.addr += 2;
  return half;
}
