auto APU::Envelope::volume() const -> u32 {
  return useSpeedAsVolume ? speed : decayVolume;
}

auto APU::Envelope::clock() -> void {
  if(reloadDecay) {
    reloadDecay = 0;
    decayVolume = 0xf;
    decayCounter = speed + 1;
    return;
  }

  if(--decayCounter == 0) {
    decayCounter = speed + 1;
    if(decayVolume || loopMode) decayVolume--;
  }
}
