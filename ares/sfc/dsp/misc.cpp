auto DSP::misc27() -> void {
  for(auto& v : voice) v._modulate = v.modulate;
}

auto DSP::misc28() -> void {
  for(auto& v : voice) v._noise = v.noise, v._echo  = v.echo;
  brr._bank = brr.bank;
}

auto DSP::misc29() -> void {
  clock.sample = !clock.sample;
  if(clock.sample) {  //clears KON 63 clocks after it was last read
    for(auto& v : voice) v._keylatch &= !v._keyon;
  }
}

auto DSP::misc30() -> void {
  if(clock.sample) {
    for(auto& v : voice) v._keyon = v._keylatch, v._keyoff = v.keyoff;
  }

  counterTick();

  //noise
  if(counterPoll(noise.frequency)) {
    int feedback = noise.lfsr << 13 ^ noise.lfsr << 14;
    noise.lfsr = feedback & 0x4000 ^ noise.lfsr >> 1;
  }
}
