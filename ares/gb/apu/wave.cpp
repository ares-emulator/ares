auto APU::Wave::getPattern(uint5 offset) const -> uint4 {
  return pattern[offset >> 1] >> (offset & 1 ? 0 : 4);
}

auto APU::Wave::run() -> void {
  if(patternHold) patternHold--;

  if(period && --period == 0) {
    period = 2048 - frequency;
    patternSample = getPattern(++patternOffset);
    patternHold = 1;
  }

  static const uint shift[] = {4, 0, 1, 2};  //0%, 100%, 50%, 25%
  uint4 sample = patternSample >> shift[volume];
  if(!enable) sample = 0;

  output = sample;
}

auto APU::Wave::clockLength() -> void {
  if(counter) {
    if(length && --length == 0) enable = false;
  }
}

auto APU::Wave::trigger() -> void {
  if(!Model::GameBoyColor() && patternHold) {
    //DMG,SGB trigger while channel is being read corrupts wave RAM
    if((patternOffset >> 1) <= 3) {
      //if current pattern is with 0-3; only byte 0 is corrupted
      pattern[0] = pattern[patternOffset >> 1];
    } else {
      //if current pattern is within 4-15; pattern&~3 is copied to pattern[0-3]
      pattern[0] = pattern[((patternOffset >> 1) & ~3) + 0];
      pattern[1] = pattern[((patternOffset >> 1) & ~3) + 1];
      pattern[2] = pattern[((patternOffset >> 1) & ~3) + 2];
      pattern[3] = pattern[((patternOffset >> 1) & ~3) + 3];
    }
  }

  enable = dacEnable;
  period = 2048 - frequency + 2;
  patternOffset = 0;
  patternSample = 0;
  patternHold = 0;

  if(!length) {
    length = 256;
    if(apu.phase.bit(0) && counter) length--;
  }
}

auto APU::Wave::readRAM(uint4 address, uint8 data) -> uint8 {
  if(enable) {
    if(!Model::GameBoyColor() && !patternHold) return data;
    return pattern[patternOffset >> 1];
  } else {
    return pattern[address];
  }
}

auto APU::Wave::writeRAM(uint4 address, uint8 data) -> void {
  if(enable) {
    if(!Model::GameBoyColor() && !patternHold) return;
    pattern[patternOffset >> 1] = data;
  } else {
    pattern[address] = data;
  }
}

auto APU::Wave::power(bool initializeLength) -> void {
  enable = 0;

  dacEnable = 0;
  volume = 0;
  frequency = 0;
  counter = 0;

  output = 0;
  period = 0;
  patternOffset = 0;
  patternSample = 0;
  patternHold = 0;

  if(initializeLength) length = 256;
}

auto APU::Wave::serialize(serializer& s) -> void {
  s.integer(enable);

  s.integer(dacEnable);
  s.integer(volume);
  s.integer(frequency);
  s.integer(counter);
  s.array(pattern);

  s.integer(output);
  s.integer(length);
  s.integer(period);
  s.integer(patternOffset);
  s.integer(patternSample);
  s.integer(patternHold);
}
