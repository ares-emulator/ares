auto PPU::color(n32 color) -> n64 {
  u32 b = color.bit(0, 3);
  u32 g = color.bit(4, 7);
  u32 r = color.bit(8,11);

  u64 R = image::normalize(r, 4, 16);
  u64 G = image::normalize(g, 4, 16);
  u64 B = image::normalize(b, 4, 16);

  if(colorEmulation->value()) {
    if(SoC::ASWAN()) {
      auto gammaAdjustBetween = [=](u32 c, u32 min, u32 max) { return min + (u32) (pow(c / 15.0, 1 / 2.2) * (max - min)); };
      // The WS's display has similar characteristics to the GBP.
      R = gammaAdjustBetween(r, 0x2b2b, 0xe0e0);
      G = gammaAdjustBetween(g, 0x2b2b, 0xdbdb);
      B = gammaAdjustBetween(b, 0x2626, 0xcdcd);
      return R << 32 | G << 16 | B << 0;
    } else {
      // The SC's display has similar characteristics to the GBC.
      R = (r * 26 + g *  4 + b *  2);
      G = (         g * 24 + b *  8);
      B = (r *  6 + g *  4 + b * 22);
      R = image::normalize(min(480, R), 9, 16);
      G = image::normalize(min(480, G), 9, 16);
      B = image::normalize(min(480, B), 9, 16);
    }
  }

  return R << 32 | G << 16 | B << 0;
}
