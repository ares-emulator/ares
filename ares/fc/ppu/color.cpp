auto PPU::color(n32 n) -> n64 {
  f64 saturation = 1.5;
  f64 hue = 0.0;
  f64 contrast = 1.0;
  f64 brightness = 1.0;
  f64 gamma = colorEmulation->value() ? 1.8 : 2.2;

  i32 color = (n & 0x0f), level = color < 0xe ? int(n >> 4 & 3) : 1;

  static constexpr f64 black = 0.518, white = 1.962, attenuation = 0.746;
  static constexpr f64 levels[8] = {
    0.350, 0.518, 0.962, 1.550,
    1.094, 1.506, 1.962, 1.962,
  };

  f64 lo_and_hi[2] = {
    levels[level + 4 * (color == 0x0)],
    levels[level + 4 * (color <  0xd)],
  };

  f64 y = 0.0, i = 0.0, q = 0.0;
  auto wave = [](i32 p, i32 color) { return (color + p + 8) % 12 < 6; };
  for(int p : range(12)) {
    f64 spot = lo_and_hi[wave(p, color)];

    if(((n & 0x040) && wave(p, 12))
    || ((n & 0x080) && wave(p,  4))
    || ((n & 0x100) && wave(p,  8))
    ) spot *= attenuation;

    f64 v = (spot - black) / (white - black);

    v = (v - 0.5) * contrast + 0.5;
    v *= brightness / 12.0;

    y += v;
    i += v * cos((Math::Pi / 6.0) * (p + hue));
    q += v * sin((Math::Pi / 6.0) * (p + hue));
  }

  i *= saturation;
  q *= saturation;

  auto gammaAdjust = [=](f64 f) { return f < 0.0 ? 0.0 : pow(f, 2.2 / gamma); };
  n64 r = uclamp<16>(65535.0 * gammaAdjust(y +  0.946882 * i +  0.623557 * q));
  n64 g = uclamp<16>(65535.0 * gammaAdjust(y + -0.274788 * i + -0.635691 * q));
  n64 b = uclamp<16>(65535.0 * gammaAdjust(y + -1.108545 * i +  1.709007 * q));

  return r << 32 | g << 16 | b << 0;
}
