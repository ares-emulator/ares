auto SPU::gaussianConstructTable() -> void {
  fesetround(FE_TONEAREST);
  f64 table[512];
  for(u32 n : range(512)) {
    f64 k = 0.5 + n;
    f64 s = (sin(Math::Pi * k * 2.048 / 1024));
    f64 t = (cos(Math::Pi * k * 2.000 / 1023) - 1) * 0.50;
    f64 u = (cos(Math::Pi * k * 4.000 / 1023) - 1) * 0.08;
    f64 r = s * (t + u + 1.0) / k;
    table[511 - n] = r;
  }
  f64 sum = 0.0;
  for(u32 n : range(512)) sum += table[n];
  f64 scale = 0x7f80 * 128 / sum;
  for(u32 n : range(512)) table[n] *= scale;
  for(u32 phase : range(128)) {
    f64 sum = 0.0;
    sum += table[phase +   0];
    sum += table[phase + 256];
    sum += table[511 - phase];
    sum += table[255 - phase];
    f64 diff = (sum - 0x7f80) / 4;
    gaussianTable[phase +   0] = nearbyint(table[phase +   0] - diff);
    gaussianTable[phase + 256] = nearbyint(table[phase + 256] - diff);
    gaussianTable[511 - phase] = nearbyint(table[511 - phase] - diff);
    gaussianTable[255 - phase] = nearbyint(table[255 - phase] - diff);
  }
}

auto SPU::Voice::gaussianRead(s8 index) const -> s32 {
  if(index < 0) return adpcm.previousSamples[index + 3];
  return adpcm.currentSamples[index];
}

auto SPU::Voice::gaussianInterpolate() const -> s32 {
  u8 g = counter >>  4 & 255;
  u8 s = counter >> 12 &  31;
  s32 out = 0;
  out += self.gaussianTable[255 - g] * gaussianRead(s - 3);
  out += self.gaussianTable[511 - g] * gaussianRead(s - 2);
  out += self.gaussianTable[256 + g] * gaussianRead(s - 1);
  out += self.gaussianTable[  0 + g] * gaussianRead(s - 0);
  return out >> 15;
}
