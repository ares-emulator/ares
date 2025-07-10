auto CPU::prefetchSync(n32 address) -> void {
  if(address == prefetch.addr) return;

  if(prefetch.wait - prefetch.cycle == 1) step(1);

  prefetch.stopped = false;
  prefetch.ahead = false;
  prefetch.addr = address;
  prefetch.load = address;
  prefetch.wait = waitCartridge(Half | Nonsequential, prefetch.load);
  prefetch.cycle = 0;
}

auto CPU::prefetchAdvance(u32 mode) -> void {
  n32 size = (mode & Word) ? 4 : 2;
  prefetch.addr += size;
  prefetch.load += size;
  prefetch.wait = waitCartridge(Half | Sequential, prefetch.load);
  prefetch.cycle = 0;
  prefetch.stopped = false;
}

auto CPU::prefetchStepInternal(u32 clocks) -> void {
  step(clocks);
  if(!wait.prefetch || prefetch.stopped) return;

  while(!prefetch.full() && clocks--) {
    prefetch.cycle++;
    if(prefetch.wait > prefetch.cycle) continue;
    prefetch.slot[prefetch.load >> 1 & 7] = cartridge.readRom<false>(Half | Nonsequential, prefetch.load);  //todo: make access sequentiality consistent with timing used
    prefetch.load += 2;
    prefetch.wait = waitCartridge(Half | Sequential, prefetch.load);
    prefetch.cycle = 0;
  }

  if(prefetch.full()) {
    prefetch.stopped = true;
    prefetch.ahead = false;
  }
}

auto CPU::prefetchStep(u32 clocks) -> void {
  if(wait.prefetch && !prefetch.stopped && prefetch.empty()) prefetch.ahead = true;
  prefetchStepInternal(clocks);
}

auto CPU::prefetchReset() -> void {
  if(prefetch.wait - prefetch.cycle == 1) step(1);

  prefetch.stopped = true;
  prefetch.ahead = false;
  prefetch.wait = 0;
  prefetch.addr = 0;
  prefetch.load = 0;
  prefetch.cycle = 0;
}

auto CPU::prefetchRead() -> n16 {
  if(prefetch.stopped && prefetch.empty()) {
    prefetch.stopped = false;
    prefetch.wait = waitCartridge(Half | Nonsequential, prefetch.load);
    prefetch.cycle = 0;
  }
  if(prefetch.empty()) prefetchStepInternal(prefetch.wait - prefetch.cycle);

  n16 word = prefetch.slot[prefetch.addr >> 1 & 7];
  prefetch.addr += 2;
  return word;
}
