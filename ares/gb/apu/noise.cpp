auto APU::Noise::dacEnable() const -> bool {
  return (envelopeVolume || envelopeDirection);
}

auto APU::Noise::getPeriod() const -> u32 {
  static const u32 table[] = {4, 8, 16, 24, 32, 40, 48, 56};
  return table[divisor] << frequency;
}

auto APU::Noise::run() -> void {
  if(period && --period == 0) {
    period = getPeriod();
    if(frequency < 14) {
      bool bit = (lfsr ^ (lfsr >> 1)) & 1;
      lfsr = (lfsr >> 1) ^ (bit << (narrow ? 6 : 14));
    }
  }

  n4 sample = lfsr & 1 ? 0 : (u32)volume;
  if(!enable) sample = 0;

  output = sample;
}

auto APU::Noise::clockLength() -> void {
  if(counter) {
    if(length && --length == 0) enable = false;
  }
}

auto APU::Noise::clockEnvelope() -> void {
  if(enable && envelopeFrequency && --envelopePeriod == 0) {
    envelopePeriod = envelopeFrequency ? (u32)envelopeFrequency : 8;
    if(envelopeDirection == 0 && volume >  0) volume--;
    if(envelopeDirection == 1 && volume < 15) volume++;
  }
}

auto APU::Noise::trigger() -> void {
  enable = dacEnable();
  lfsr = -1;
  envelopePeriod = envelopeFrequency ? (u32)envelopeFrequency : 8;
  volume = envelopeVolume;

  if(!length) {
    length = 64;
    if(apu.phase.bit(0) && counter) length--;
  }
}

auto APU::Noise::power(bool initializeLength) -> void {
  enable = 0;

  envelopeVolume = 0;
  envelopeDirection = 0;
  envelopeFrequency = 0;
  frequency = 0;
  narrow = 0;
  divisor = 0;
  counter = 0;

  output = 0;
  envelopePeriod = 0;
  volume = 0;
  period = 0;
  lfsr = 0;

  if(initializeLength) length = 64;
}

auto APU::Noise::serialize(serializer& s) -> void {
  s(enable);

  s(envelopeVolume);
  s(envelopeDirection);
  s(envelopeFrequency);
  s(frequency);
  s(narrow);
  s(divisor);
  s(counter);

  s(output);
  s(length);
  s(envelopePeriod);
  s(volume);
  s(period);
  s(lfsr);
}
