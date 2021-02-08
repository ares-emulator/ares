auto PPU::colorGameBoy(n32 color) -> n64 {
  if(colorEmulationDMG->value() == "Game Boy") {
    const n8 monochrome[4][3] = {
      {0xae, 0xd9, 0x27},
      {0x58, 0xa0, 0x28},
      {0x20, 0x62, 0x29},
      {0x1a, 0x45, 0x2a},
    };
    n64 R = monochrome[color][0] * 0x0101;
    n64 G = monochrome[color][1] * 0x0101;
    n64 B = monochrome[color][2] * 0x0101;
    return R << 32 | G << 16 | B << 0;
  }

  if(colorEmulationDMG->value() == "Game Boy Pocket") {
    const n8 monochrome[4][3] = {
      {0xe0, 0xdb, 0xcd},
      {0xa8, 0x9f, 0x94},
      {0x70, 0x6b, 0x66},
      {0x2b, 0x2b, 0x26},
    };
    n64 R = monochrome[color][0] * 0x0101;
    n64 G = monochrome[color][1] * 0x0101;
    n64 B = monochrome[color][2] * 0x0101;
    return R << 32 | G << 16 | B << 0;
  }

  if(colorEmulationDMG->value() == "RGB") {
    const n8 monochrome[4][3] = {
      {0xff, 0xff, 0xff},
      {0xaa, 0xaa, 0xaa},
      {0x55, 0x55, 0x55},
      {0x00, 0x00, 0x00},
    };
    n64 R = monochrome[color][0] * 0x0101;
    n64 G = monochrome[color][1] * 0x0101;
    n64 B = monochrome[color][2] * 0x0101;
    return R << 32 | G << 16 | B << 0;
  }

  return 0;
}

auto PPU::colorGameBoyColor(n32 color) -> n64 {
  n32 r = color.bit( 0, 4);
  n32 g = color.bit( 5, 9);
  n32 b = color.bit(10,14);

  n64 R = image::normalize(r, 5, 16);
  n64 G = image::normalize(g, 5, 16);
  n64 B = image::normalize(b, 5, 16);

  if(colorEmulationCGB->value()) {
    R = (r * 26 + g *  4 + b *  2);
    G = (         g * 24 + b *  8);
    B = (r *  6 + g *  4 + b * 22);
    R = image::normalize(min(960, R), 10, 16);
    G = image::normalize(min(960, G), 10, 16);
    B = image::normalize(min(960, B), 10, 16);
  }

  return R << 32 | G << 16 | B << 0;
}
