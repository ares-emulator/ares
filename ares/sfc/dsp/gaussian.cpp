auto DSP::gaussianConstructTable() -> void {
  double table[512];
  for(uint n : range(512)) {
    double k = 0.5 + n;
    double s = (sin(Math::Pi * k * 1.280 / 1024));
    double t = (cos(Math::Pi * k * 2.000 / 1023) - 1) * 0.50;
    double u = (cos(Math::Pi * k * 4.000 / 1023) - 1) * 0.08;
    double r = s * (t + u + 1.0) / k;
    table[511 - n] = r;
  }
  for(uint phase : range(128)) {
    double sum = 0.0;
    sum += table[phase +   0];
    sum += table[phase + 256];
    sum += table[511 - phase];
    sum += table[255 - phase];
    double scale = 2048.0 / sum;
    gaussianTable[phase +   0] = int16(table[phase +   0] * scale + 0.5);
    gaussianTable[phase + 256] = int16(table[phase + 256] * scale + 0.5);
    gaussianTable[511 - phase] = int16(table[511 - phase] * scale + 0.5);
    gaussianTable[255 - phase] = int16(table[255 - phase] * scale + 0.5);
  }
}

auto DSP::gaussianInterpolate(const Voice& v) -> int {
  //make pointers into gaussian table based on fractional position between samples
  uint8 offset = v.gaussianOffset >> 4;
  const int16* forward = gaussianTable + 255 - offset;
  const int16* reverse = gaussianTable       + offset;  //mirror left half of gaussian table

  offset = (v.bufferOffset + (v.gaussianOffset >> 12)) % 12;
  int output;
  output  = forward[  0] * v.buffer[offset] >> 11; if(++offset >= 12) offset = 0;
  output += forward[256] * v.buffer[offset] >> 11; if(++offset >= 12) offset = 0;
  output += reverse[256] * v.buffer[offset] >> 11; if(++offset >= 12) offset = 0;
  output  = int16(output);
  output += reverse[  0] * v.buffer[offset] >> 11;
  return sclamp<16>(output) & ~1;
}
