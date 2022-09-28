auto YM2610::PCMA::power() -> void {
  for(auto c : range(6)) {
    channels[c].startAddress.bit(0, 7) = 0x00;
    channels[c].endAddress.bit(0, 7) = 0x00;
    channels[c].decodeAccumulator = 0;
    channels[c].decodeStep = 0;
  }

  i16 stepSize[] = {
    16, 17, 19, 21, 23, 25, 28, 31, 34, 37,
    41, 45, 50, 55, 60, 66, 73, 80, 88, 97,
    107, 118, 130, 143, 157, 173, 190, 209, 230, 253,
    279, 307, 337, 371, 408, 449, 494, 544, 598, 658,
    724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552
  };

  // Generate a decode table
  for (int step = 0; step < 49; step++) {
    for (int nibble = 0; nibble < 16; nibble++) {
      int value = (2 * (nibble & 0x07) + 1) * stepSize[step] / 8;
      decodeTable[step * 16 + nibble] = ((nibble & 0x08) != 0) ? -value : value;
    }
  }
}

auto YM2610::PCMA::clock() -> array<i16[2]> {
  f32 left = 0;
  f32 right = 0;

   for(auto c : range(6)) {
    if(!channels[c].playing) continue;
    if(((channels[c].currentAddress ^ (channels[c].endAddress + (1 << 8))) & 0xfffff) == 0) {
      channels[c].playing = 0;
      continue;
    }

    auto sample = channels[c].decode(self.readPCMA(channels[c].currentAddress));

    int totalVolume = (volume ^ 0x3f) + (channels[c].volume ^ 0x1f);
    int volumeMultiplier = 15 - (totalVolume & 7);
    int volumeShift = 5 + (totalVolume >> 3);

    sample = ((sample * volumeMultiplier) >> volumeShift) & ~3;

    if(channels[c].left)   left += sample / 2047.0f;
    if(channels[c].right) right += sample / 2047.0f;
  }

  return {sclamp<16>(left * 32768.0), sclamp<16>(right * 32768.0)};
}

auto YM2610::PCMA::Channel::keyOn() -> void {
  playing = 1;
  currentAddress = startAddress;
  decodeAccumulator = 0;
  decodeStep = 0;
  currentNibble = 0;
}

auto YM2610::PCMA::Channel::keyOff() -> void {
  playing = 0;
}

auto YM2610::PCMA::Channel::decode(n8 value) -> i12 {
  int stepAdjust[] = { -1, -1, -1, -1, 2, 5, 7, 9, -1, -1, -1, -1, 2, 5, 7, 9 };

  value = currentNibble ? value.bit(0, 3) : value.bit(4, 7);
  if(currentNibble) currentAddress++;
  currentNibble = !currentNibble;

  decodeAccumulator += self.decodeTable[decodeStep + value];

  decodeStep += stepAdjust[value & 7] * 16;
  decodeStep = std::clamp((int)decodeStep, 0, 48 * 16);

  return decodeAccumulator;
}

auto YM2610::PCMB::power() -> void {
  currentNibble       = 0;
  currentAddress      = 0;
  decodeAccumulator   = 0;
  previousAccumulator = 0;
  decodePosition      = 0;
  decodeStep          = 127;
  volume              = 255;
}

auto YM2610::PCMB::clock() -> array<i16[2]> {
  f32 leftSample = 0;
  f32 rightSample = 0;

  decode();

  i32 sample = (previousAccumulator * (i32)((decodePosition ^ 0xffff) + 1) + decodeAccumulator * (i32)(decodePosition)) >> 16;
  sample = (sample * (i32)volume) >> 9;

  if(left)  leftSample  += (sample / 65535.0);
  if(right) rightSample += (sample / 65535.0);

  return {sclamp<16>(leftSample * 32768.0), sclamp<16>(rightSample * 32768.0)};
}

auto YM2610::PCMB::decode() -> void {
  if(!playing) {
    return;
  }

  u8 stepSize[8] = { 57, 57, 57, 57, 77, 102, 128, 153 };

  u32 position = decodePosition + delta;
  decodePosition = (u16)position;
  if(position < 0x10000) return;

  n8 value = self.readPCMB(currentAddress);
  value = currentNibble ? value.bit(0, 3) : value.bit(4, 7);
  currentNibble = !currentNibble;

  if(currentNibble == 0) {
    currentAddress++;
    if(currentAddress == endAddress) {
      if(repeat) beginPlay();
      else {
        decodeAccumulator = 0;
        previousAccumulator = 0;
        playing = 0;
      }
    }
  }

  previousAccumulator = decodeAccumulator;

  i32 _delta = (value.bit(0, 2) * 2 + 1) * decodeStep / 8;
  if(value.bit(3)) _delta = -_delta;

  decodeAccumulator = sclamp<16>(decodeAccumulator + _delta);

  decodeStep = std::clamp(((int)decodeStep * stepSize[value.bit(0, 2)]) / 64, 127, 24576);
}

auto YM2610::PCMB::beginPlay() -> void {
  currentAddress      = startAddress;
  currentNibble       = 0;
  decodeAccumulator   = 0;
  previousAccumulator = 0;
  decodePosition      = 0;
  decodeStep          = 127;
  playing             = 1;
}