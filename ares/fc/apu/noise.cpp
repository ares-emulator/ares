auto APU::Noise::clockLength() -> void {
  if(envelope.loopMode == 0) {
    if(lengthCounter > 0) lengthCounter--;
  }
}

auto APU::Noise::clock() -> n8 {
  if(lengthCounter == 0) return 0;

  n8 result = (lfsr & 1) ? envelope.volume() : 0;

  if(--periodCounter == 0) {
    u32 feedback;

    if(shortMode) {
      feedback = ((lfsr >> 0) & 1) ^ ((lfsr >> 6) & 1);
    } else {
      feedback = ((lfsr >> 0) & 1) ^ ((lfsr >> 1) & 1);
    }

    lfsr = (lfsr >> 1) | (feedback << 14);
    periodCounter = Region::PAL() ? apu.noisePeriodTablePAL[period] : apu.noisePeriodTableNTSC[period];
  }

  return result;
}
