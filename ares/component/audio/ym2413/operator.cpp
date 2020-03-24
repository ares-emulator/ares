auto YM2413::Operator::releaseRate() const -> uint4 {
  if(state == Trigger) return 13;
  if(state == Attack) return attack;
  if(state == Decay) return decay;
  if(state == Sustain) return sustainable ? (uint4)0 : release;
  // state == Release
  if(!audible) return 0;  //should never happen
  if(sustainOn) return 5;  //carrier only
  return sustainable ? release : (uint4)7;
}

auto YM2413::Operator::update(maybe<uint> updateState) -> void {
  if(updateState) state = updateState();

  auto baseRate = releaseRate();
  auto ksr = 2 * block + fnumber.bit(8) >> (scaleRate ? 0 : 2);

  rate = min(60, baseRate ? 4 * baseRate + ksr : 0);
  divider = max(state == Attack ? 2 : 0, 13 - rate / 4);
  sequence = envelopeSteps[rate < 48 ? rate % 4 : rate - 44][state == Attack ? 0 : 1];

  auto ksl = levelScaling[fnumber.bit(5,8)] + 8 * block - 56;
  level  = totalLevel;
  level += max(0, scaleLevel ? ksl : 0) >> 3 - scaleLevel;
  auto f = fnumber.bit(6,8);
  long m = multiple;
  long lfo[8]{0, -f/2, -f, -f/2, 0, +f/2, +f, +f/2};

  m += (m == 14) - (m == 13 || m == 11);  //unimplemented; maps to 15, 12, or 10
  m  = max(1, 2 * m);  //0 => 1/2, which truncates fnumber bit 0

  for(uint n : range(8)) {
    pitch[n] = (fnumber + lfo[n]) * m / 2 << block;
  }
}

auto YM2413::Operator::synchronize(uint1 hard, maybe<Operator&> modulator) -> void {
  if(state == Trigger && envelope.bit(2,6) == 31) {
    update(Attack);
    if(rate >= 60) update(Decay), envelope = 0;
    if(hard) phase = 0;
    if(modulator) {
      modulator->update(Attack);
      if(modulator->rate >= 60) modulator->update(Decay), modulator->envelope = 0;
      if(hard) modulator->phase = 0;
    }
  }
  if(state == Attack && envelope == 0x00) update(Decay);
  if(state == Decay && envelope >= sustain << 3) update(Sustain);
  if(modulator) modulator->synchronize(hard);
}

//only carriers (never modulators) call trigger()
auto YM2413::Operator::trigger(uint1 key, uint1 sustain) -> void {
  sustainOn = sustain;
  if(keyOn == key) return;
  keyOn = key;
  update(keyOn ? Trigger : Release);
}

auto YM2413::Operator::clock(natural clock, integer offset, integer modulation) -> int12 {
  uint14 y = YM2413::sinTable[uint10(offset + modulation)];
  if(waveform & ~y.bit(0)) y = sinTable[0];
  auto w = clock / 64 % 210 * tremolo;
  auto lfo = (w < 105 ? w : 210 - w) >> 3;
  y += envelope << 4 + 1;
  y += level    << 5 + 1;
  y += lfo      << 4 + 1;
  prior = output;
  output = expTable[y.bit(0,8)] * 1L >> y.bit(9,13);
  phase += pitch[clock / 1024 % 8 * vibrato];
  auto lsbs = divider ? clock.bit(0, divider - 1) : 0;
  if(rate && (state == Attack ? lsbs & -4 : lsbs) == 0) {
    natural msbs = uint4(clock >> divider);
    integer step = uint4(sequence << 4 * msbs >> 60);
    if(state == Attack) step = ~envelope * step >> 4;
    envelope = max(0x00, min(0x7f, envelope + step));
  }
  return output;
}
