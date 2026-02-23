auto DSP::gaussianConstructTable() -> void {
  f64 table[512];
  for(u32 n : range(512)) {
    f64 k = 0.5 + n;
    f64 s = (sin(Math::Pi * k * 1.280 / 1024));
    f64 t = (cos(Math::Pi * k * 2.000 / 1023) - 1) * 0.50;
    f64 u = (cos(Math::Pi * k * 4.000 / 1023) - 1) * 0.08;
    f64 r = s * (t + u + 1.0) / k;
    table[511 - n] = r;
  }
  for(u32 phase : range(128)) {
    f64 sum = 0.0;
    sum += table[phase +   0];
    sum += table[phase + 256];
    sum += table[511 - phase];
    sum += table[255 - phase];
    f64 scale = 2048.0 / sum;
    gaussianTable[phase +   0] = i16(table[phase +   0] * scale + 0.5);
    gaussianTable[phase + 256] = i16(table[phase + 256] * scale + 0.5);
    gaussianTable[511 - phase] = i16(table[511 - phase] * scale + 0.5);
    gaussianTable[255 - phase] = i16(table[255 - phase] * scale + 0.5);
  }
}

auto DSP::gaussianInterpolate(const Voice& v) -> s32 {
  //make pointers into gaussian table based on fractional position between samples
  n8 offset = v.gaussianOffset >> 4;
  const i16* forward = gaussianTable + 255 - offset;
  const i16* reverse = gaussianTable       + offset;  //mirror left half of gaussian table

  offset = (v.bufferOffset + (v.gaussianOffset >> 12)) % 12;
  s32 output;
  output  = forward[  0] * v.buffer[offset] >> 11; if(++offset >= 12) offset = 0;
  output += forward[256] * v.buffer[offset] >> 11; if(++offset >= 12) offset = 0;
  output += reverse[256] * v.buffer[offset] >> 11; if(++offset >= 12) offset = 0;
  output  = i16(output);
  output += reverse[  0] * v.buffer[offset] >> 11;
  return sclamp<16>(output) & ~1;
}
