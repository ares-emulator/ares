auto SH2::FRT::run() -> void {
  static constexpr u32 frequencies[4] = {8, 32, 128, 1};

  if(++counter >= frequencies[tcr.cks]) {
    counter -= frequencies[tcr.cks];
    if(++frc == 0) {
      ftcsr.ovf = 1;
      if(tier.ovie) {
        pendingOutputIRQ = 1;
      }
    }
    if(frc == ocra) {
      ftcsr.ocfa = 1;
      if(tier.ociae) {
        pendingOutputIRQ = 1;
      }
      if(ftcsr.cclra) {
        frc = 0;
      }
    }
    if(frc == ocrb) {
      ftcsr.ocfb = 1;
      if(tier.ocibe) {
        pendingOutputIRQ = 1;
      }
    }
  }
}
