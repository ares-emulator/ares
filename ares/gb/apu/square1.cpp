auto APU::Square1::dacEnable() const -> bool {
  return (envelopeVolume || envelopeDirection);
}

auto APU::Square1::run() -> void {
  if(period && --period == 0) {
    period = 2 * (2048 - frequency);
    phase++;
    switch(duty) {
    case 0: dutyOutput = (phase == 6); break;  //______-_
    case 1: dutyOutput = (phase >= 6); break;  //______--
    case 2: dutyOutput = (phase >= 4); break;  //____----
    case 3: dutyOutput = (phase <= 5); break;  //------__
    }
  }

  uint4 sample = dutyOutput ? (uint)volume : 0;
  if(!enable) sample = 0;

  output = sample;
}

auto APU::Square1::sweep(bool update) -> void {
  if(!sweepEnable) return;

  sweepNegate = sweepDirection;
  uint delta = frequencyShadow >> sweepShift;
  int freq = frequencyShadow + (sweepNegate ? -delta : delta);

  if(freq > 2047) {
    enable = false;
  } else if(sweepShift && update) {
    frequencyShadow = freq;
    frequency = freq & 2047;
    period = 2 * (2048 - frequency);
  }
}

auto APU::Square1::clockLength() -> void {
  if(counter) {
    if(length && --length == 0) enable = false;
  }
}

auto APU::Square1::clockSweep() -> void {
  if(--sweepPeriod == 0) {
    sweepPeriod = sweepFrequency ? (uint)sweepFrequency : 8;
    if(sweepEnable && sweepFrequency) {
      sweep(1);
      sweep(0);
    }
  }
}

auto APU::Square1::clockEnvelope() -> void {
  if(enable && envelopeFrequency && --envelopePeriod == 0) {
    envelopePeriod = envelopeFrequency ? (uint)envelopeFrequency : 8;
    if(envelopeDirection == 0 && volume >  0) volume--;
    if(envelopeDirection == 1 && volume < 15) volume++;
  }
}

auto APU::Square1::trigger() -> void {
  enable = dacEnable();
  period = 2 * (2048 - frequency);
  envelopePeriod = envelopeFrequency ? (uint)envelopeFrequency : 8;
  volume = envelopeVolume;

  if(!length) {
    length = 64;
    if(apu.phase.bit(0) && counter) length--;
  }

  frequencyShadow = frequency;
  sweepNegate = false;
  sweepPeriod = sweepFrequency ? (uint)sweepFrequency : 8;
  sweepEnable = sweepPeriod || sweepShift;
  if(sweepShift) sweep(0);
}

auto APU::Square1::power(bool initializeLength) -> void {
  enable = 0;

  sweepFrequency = 0;
  sweepDirection = 0;
  sweepShift = 0;
  sweepNegate = 0;
  duty = 0;
  envelopeVolume = 0;
  envelopeDirection = 0;
  envelopeFrequency = 0;
  frequency = 0;
  counter = 0;

  output = 0;
  dutyOutput = 0;
  phase = 0;
  period = 0;
  envelopePeriod = 0;
  sweepPeriod = 0;
  frequencyShadow = 0;
  sweepEnable = 0;
  volume = 0;

  if(initializeLength) length = 64;
}

auto APU::Square1::serialize(serializer& s) -> void {
  s.integer(enable);

  s.integer(sweepFrequency);
  s.integer(sweepDirection);
  s.integer(sweepShift);
  s.integer(sweepNegate);
  s.integer(duty);
  s.integer(length);
  s.integer(envelopeVolume);
  s.integer(envelopeDirection);
  s.integer(envelopeFrequency);
  s.integer(frequency);
  s.integer(counter);

  s.integer(output);
  s.integer(dutyOutput);
  s.integer(phase);
  s.integer(period);
  s.integer(envelopePeriod);
  s.integer(sweepPeriod);
  s.integer(frequencyShadow);
  s.integer(sweepEnable);
  s.integer(volume);
}
