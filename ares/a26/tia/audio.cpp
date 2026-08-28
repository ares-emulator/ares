auto TIA::Audio::load(Node::Object parent, f64 frequency) -> void {
  stream = parent->append<Node::Audio::Stream>("Audio");
  stream->setChannels(1);
  stream->setFrequency(frequency / 114.0);
}

auto TIA::Audio::unload(Node::Object parent) -> void {
  parent->remove(stream);
  stream.reset();
}

auto TIA::Audio::clock() -> void {
  auto conductance = dacConductance(channel[0].output()) + dacConductance(channel[1].output());
  sum += loadedOutput(conductance);
  clocks++;

  if(phase == 9 || phase == 81) {
    channel[0].phase0();
    channel[1].phase0();
  }

  if(phase == 37 || phase == 149) {
    channel[0].phase1();
    channel[1].phase1();

    //TIA-1A uses binary-weighted DACs; NTSC mixes both channels through 1k.
    stream->frame(sum / clocks);
    sum = 0;
    clocks = 0;
  }
  advance();
}

auto TIA::Audio::dacConductance(u8 code) const -> f64 {
  return code / 15.0;
}

auto TIA::Audio::loadedOutput(f64 conductance) const -> f64 {
  return conductance ? 2.0 * conductance / (2.0 + conductance) : 0.0;
}

auto TIA::Audio::Channel::phase0() -> void {
  if (enable) {
    switch (control.bit(0, 1)) {
    case 0x00: case 0x01: pulseCounterPaused = 0;                break;
    case 0x02: pulseCounterPaused = noiseCounter.bit(1, 4) != 1; break;
    case 0x03: pulseCounterPaused = !noiseCounter.bit(0);        break;
    }

    noiseFeedback = (noiseCounter.bit(2) ^ noiseCounter.bit(0)) || noiseCounter == 0;
    if(control.bit(0, 1) == 0) {
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

auto TIA::Audio::Channel::phase1() -> void {
  if(!enable) return;

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
}

auto TIA::Audio::Channel::output() const -> u8 {
  return pulseCounter.bit(0) * volume;
}

auto TIA::Audio::power() -> void {
  channel[0] = {};
  channel[1] = {};
  phase = 0;
  sum = 0;
  clocks = 0;
}
