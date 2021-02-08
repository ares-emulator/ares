auto DSP::calculateFIR(n1 channel, s32 index) -> s32 {
  s32 sample = echo.history[channel][(n3)(echo._historyOffset + index + 1)];
  return (sample * echo.fir[index]) >> 6;
}

auto DSP::echoOutput(n1 channel) const -> i16 {
  i16 masterOutput = master.output[channel] * master.volume[channel] >> 7;
    i16 echoOutput =    echo.input[channel] *   echo.volume[channel] >> 7;
  return sclamp<16>(masterOutput + echoOutput);
}

auto DSP::echoRead(n1 channel) -> void {
  n16 address = echo._address + channel * 2;
  n8 lo = apuram[address++];
  n8 hi = apuram[address++];
  s32 s = (i16)((hi << 8) + lo);
  echo.history[channel][echo._historyOffset] = s >> 1;
}

auto DSP::echoWrite(n1 channel) -> void {
  if(!echo._readonly) {
    n16 address = echo._address + channel * 2;
    auto sample = echo.output[channel];
    apuram[address++] = sample.byte(0);
    apuram[address++] = sample.byte(1);
  }
  echo.output[channel] = 0;
}

auto DSP::echo22() -> void {
  //history
  echo._historyOffset++;

  echo._address = (echo._bank << 8) + echo._offset;
  echoRead(0);

  //FIR
  s32 l = calculateFIR(0, 0);
  s32 r = calculateFIR(1, 0);

  echo.input[0] = l;
  echo.input[1] = r;
}

auto DSP::echo23() -> void {
  s32 l = calculateFIR(0, 1) + calculateFIR(0, 2);
  s32 r = calculateFIR(1, 1) + calculateFIR(1, 2);

  echo.input[0] += l;
  echo.input[1] += r;

  echoRead(1);
}

auto DSP::echo24() -> void {
  s32 l = calculateFIR(0, 3) + calculateFIR(0, 4) + calculateFIR(0, 5);
  s32 r = calculateFIR(1, 3) + calculateFIR(1, 4) + calculateFIR(1, 5);

  echo.input[0] += l;
  echo.input[1] += r;
}

auto DSP::echo25() -> void {
  s32 l = echo.input[0] + calculateFIR(0, 6);
  s32 r = echo.input[1] + calculateFIR(1, 6);

  l = (i16)l;
  r = (i16)r;

  l += (i16)calculateFIR(0, 7);
  r += (i16)calculateFIR(1, 7);

  echo.input[0] = sclamp<16>(l) & ~1;
  echo.input[1] = sclamp<16>(r) & ~1;
}

auto DSP::echo26() -> void {
  //left output volumes
  //(save sample for next clock so we can output both together)
  master.output[0] = echoOutput(0);

  //echo feedback
  s32 l = echo.output[0] + i16(echo.input[0] * echo.feedback >> 7);
  s32 r = echo.output[1] + i16(echo.input[1] * echo.feedback >> 7);

  echo.output[0] = sclamp<16>(l) & ~1;
  echo.output[1] = sclamp<16>(r) & ~1;
}

auto DSP::echo27() -> void {
  s32 outl = master.output[0];
  s32 outr = echoOutput(1);
  master.output[0] = 0;
  master.output[1] = 0;

  //todo: global muting isn't this simple
  //(turns DAC on and off or something, causing small ~37-sample pulse when first muted)
  if(master.mute) {
    outl = 0;
    outr = 0;
  }

  //output sample to DAC
  sample(outl, outr);
}

auto DSP::echo28() -> void {
  echo._readonly = echo.readonly;
}

auto DSP::echo29() -> void {
  echo._bank = echo.bank;

  if(!echo._offset) echo._length = echo.delay << 11;

  echo._offset += 4;
  if(echo._offset >= echo._length) echo._offset = 0;

  //write left echo
  echoWrite(0);

  echo._readonly = echo.readonly;
}

auto DSP::echo30() -> void {
  //write right echo
  echoWrite(1);
}
