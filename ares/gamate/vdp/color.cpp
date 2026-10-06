//the panel is a green STN much like the Mega Duck's, and MAME reuses that
//machine's measured colours here
auto VDP::color(n32 color) -> n64 {
  static const n8 shades[4][3] = {
    {0x6b, 0xa6, 0x4a},
    {0x43, 0x7a, 0x63},
    {0x25, 0x59, 0x55},
    {0x12, 0x42, 0x4c},
  };
  n64 R = shades[color][0] * 0x0101;
  n64 G = shades[color][1] * 0x0101;
  n64 B = shades[color][2] * 0x0101;
  return R << 32 | G << 16 | B << 0;
}
