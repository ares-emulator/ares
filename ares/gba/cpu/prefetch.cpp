auto CPU::prefetchSync(u32 mode, n32 address) -> void {
  if(prefetch.wait == 1) step(1);

  n32 size = (mode & Word) ? 4 : 2;
  prefetch.stopped = false;
  prefetch.ahead = false;
  prefetch.addr = address + size;
  prefetch.load = address + size;
  prefetch.wait = waitCartridge(prefetch.load, true);
}

auto CPU::prefetchStep(u32 clocks) -> void {
  step(clocks);
  if(!wait.prefetch || prefetch.stopped) return;

  while(clocks--) {
    if(prefetch.full()) {
      prefetch.stopped = true;
      prefetch.ahead = false;
      break;
    }
    prefetch.ahead = true;
    if(--prefetch.wait) continue;
    if(prefetch.load & 0x1fffe) {
      cartridge.startBurst(prefetch.load);  //todo: only start burst when necessary
      prefetch.slot[prefetch.load >> 1 & 7] = cartridge.readRom<false>(prefetch.load);
      prefetch.load += 2;
    }
    prefetch.wait = waitCartridge(prefetch.load, true);
  }
}

auto CPU::prefetchReset() -> void {
  if(prefetch.wait == 1) step(1);

  prefetch.stopped = true;
  prefetch.ahead = false;
  prefetch.wait = 0;
  prefetch.addr = 0;
  prefetch.load = 0;
}

auto CPU::prefetchRead() -> n16 {
  if(prefetch.stopped && prefetch.empty()) {
    prefetch.stopped = false;
    prefetch.wait = waitCartridge(prefetch.load, false);
  }
  if(prefetch.empty()) prefetchStep(prefetch.wait);

  n16 word = prefetch.slot[prefetch.addr >> 1 & 7];
  prefetch.addr += 2;
  return word;
}
