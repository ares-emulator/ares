auto TIA::runAudio() -> void {
  if(io.hcounter == 9 || io.hcounter == 81) {
    audio[0].phase0();
    audio[1].phase0();
  }

  if(io.hcounter == 37 || io.hcounter == 149) {
    auto a = audio[0].phase1();
    auto b = audio[1].phase1();

    double output = 0;
    output += volume[a];
    output += volume[b];

    stream->frame(output / 2.0);
  }
}

auto TIA::AudioChannel::phase0() -> void {
  if (enable) {
    switch (control.bit(0, 1)) {
    case 0x00: case 0x01: pulseCounterPaused = 0;                break;
    case 0x02: pulseCounterPaused = noiseCounter.bit(1, 4) != 1; break;
    case 0x03: pulseCounterPaused = !noiseCounter.bit(0);        break;
    }

    noiseFeedback = (noiseCounter.bit(2) ^ noiseCounter.bit(0)) || noiseCounter == 0;
    if(control.bit(0, 2) == 0) {
      noiseFeedback = !(control.bit(2, 3)) ||
                      !(noiseCounter || (pulseCounter != 0x0a)) ||
                      (pulseCounter.bit(0) ^ noiseCounter.bit(0));
    }
  }

  enable = divCounter == frequency;
  if (divCounter == frequency || divCounter == 0x1f) {
    divCounter = 0;
    return;
  }

  divCounter++;
}

auto TIA::AudioChannel::phase1() -> u8 {
  if(!enable) return pulseCounter.bit(0) * volume;

  pulseFeedback = 0;
  switch(control.bit(2, 3)) {
    case 0x00: pulseFeedback = pulseCounter != 0x0a && control.bit(0, 2) && (pulseCounter.bit(1) ^ pulseCounter.bit(0)); break;
    case 0x01: pulseFeedback = !pulseCounter.bit(3);                                                                   ; break;
    case 0x02: pulseFeedback = !noiseCounter.bit(0);                                                                   ; break;
    case 0x03: pulseFeedback = !(pulseCounter.bit(1) || !pulseCounter.bit(1, 3));                                      ; break;
  }

  noiseCounter >>= 1;
  if (noiseFeedback) noiseCounter.bit(4) = 1;

  if(!pulseCounterPaused) {
    pulseCounter = ~(pulseCounter >> 1) & 7;
    if (pulseFeedback) pulseCounter.bit(3) = 1;
  }

  return pulseCounter.bit(0) * volume;
}
