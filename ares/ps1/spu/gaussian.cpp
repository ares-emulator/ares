auto SPU::gaussianConstructTable() -> void {
  fesetround(FE_TONEAREST);
  double table[512];
  for(uint n : range(512)) {
    double k = 0.5 + n;
    double s = (sin(Math::Pi * k * 2.048 / 1024));
    double t = (cos(Math::Pi * k * 2.000 / 1023) - 1) * 0.50;
    double u = (cos(Math::Pi * k * 4.000 / 1023) - 1) * 0.08;
    double r = s * (t + u + 1.0) / k;
    table[511 - n] = r;
  }
  double sum = 0.0;
  for(uint n : range(512)) sum += table[n];
  double scale = 0x7f80 * 128 / sum;
  for(uint n : range(512)) table[n] *= scale;
  for(uint phase : range(128)) {
    double sum = 0.0;
    sum += table[phase +   0];
    sum += table[phase + 256];
    sum += table[511 - phase];
    sum += table[255 - phase];
    double diff = (sum - 0x7f80) / 4;
    gaussianTable[phase +   0] = nearbyint(table[phase +   0] - diff);
    gaussianTable[phase + 256] = nearbyint(table[phase + 256] - diff);
    gaussianTable[511 - phase] = nearbyint(table[511 - phase] - diff);
    gaussianTable[255 - phase] = nearbyint(table[255 - phase] - diff);
  }
}

auto SPU::Voice::gaussianRead(i8 index) const -> i32 {
  if(index < 0) return adpcm.previousSamples[index + 3];
  return adpcm.currentSamples[index];
}

auto SPU::Voice::gaussianInterpolate() const -> i32 {
  u8 g = counter >>  4 & 255;
  u8 s = counter >> 12 &  31;
  i32 out = 0;
  out += self.gaussianTable[255 - g] * gaussianRead(s - 3);
  out += self.gaussianTable[511 - g] * gaussianRead(s - 2);
  out += self.gaussianTable[256 + g] * gaussianRead(s - 1);
  out += self.gaussianTable[  0 + g] * gaussianRead(s - 0);
  return out >> 15;
}
