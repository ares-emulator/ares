auto ANTIC::Interrupt::power() -> void {
  pending = 0;
  enable = 0;
  pulse = 0;
}

auto ANTIC::Interrupt::schedule(n8 source) -> void {
  pending = source;
}

auto ANTIC::Interrupt::clock() -> void {
  if(self.counter.machineCycle == 7 && pending) {
    // VBI and DLI status are mutually exclusive and latch independently of
    // NMIEN. The status becomes visible one cycle before the NMI pulse.
    if(pending == 0x40) self.io.nmist &= ~0x80;
    if(pending == 0x80) self.io.nmist &= ~0x40;
    self.io.nmist |= pending;
    // AHRM 4.8: enabling closes at cycle 7, while a cycle-8 write can still
    // disable an interrupt that was enabled at the earlier boundary.
    enable = self.io.nmien;
  }
  if(self.counter.machineCycle == 8 && pending) {
    if(pending & enable & self.io.nmien) {
      cpu.nmiLine(1);
      pulse = 1;
    }
  }
  if(self.counter.machineCycle == 9) {
    if(pulse) cpu.nmiLine(0);
    pulse = 0;
    pending = 0;
    enable = 0;
  }
}

auto ANTIC::WSYNC::power() -> void {
  active = 0;
  scanline = 0;
}

auto ANTIC::WSYNC::clock() -> void {
  if(active && scanline == self.counter.scanline && self.counter.machineCycle == Timing::WSYNCReleaseCycle) {
    active = 0;
    cpu.rdyLine(1);
  }
}

auto ANTIC::WSYNC::wait() -> void {
  active = 1;
  scanline = self.counter.scanline + (self.counter.machineCycle >= 104);
  if(scanline == Timing::ScanlinesPerFrame) scanline = 0;
  cpu.rdyLine(0);
}
