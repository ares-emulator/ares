auto SH2::FRT::run(int cycles) -> void {
  static constexpr u32 frequencies[4] = {8, 32, 128, 1};
  auto frequency = frequencies[tcr.cks];

  counter += cycles;
  auto increments = counter / frequency;
  counter -= increments * frequency;

  if (!increments) return;

  auto prevFrc = frc;
  frc += increments;

  ftcsr.ovf  |= (frc < prevFrc);
  ftcsr.ocfa |= (u32(ocra - 1 - prevFrc) < increments);
  ftcsr.ocfb |= (u32(ocrb - 1 - prevFrc) < increments);

  pendingOutputIRQ |= (tier.ovie & ftcsr.ovf) |
                      (tier.ociae & ftcsr.ocfa) |
                      (tier.ocibe & ftcsr.ocfb);

  if (ftcsr.cclra && ftcsr.ocfa) frc = 0;
}

auto SH2::WDT::run(int cycles) -> void {
  static constexpr u32 frequencies[8] = {2, 64, 128, 256, 512, 1024, 4096, 8192};

  if(!wtcsr.tme || wtcsr.wtit) return; //watchdog mode (wtit) currently unsupported

  auto frequency = frequencies[wtcsr.cks];

  counter += cycles;
  auto increments = counter / frequency;
  counter %= frequency;

  if(!increments) return;

  auto prevWtcnt = wtcnt;
  wtcnt += increments;

  if (wtcnt < prevWtcnt) {
    wtcsr.ovf = 1;
    pendingIRQ = 1;
  }
}
