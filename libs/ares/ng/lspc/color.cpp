auto LSPC::color(n32 color) -> n64 {
  n32 b = color.bit(15) << 0 | color.bit(12) << 1 | color.bit(0, 3) << 2;
  n32 g = color.bit(15) << 0 | color.bit(13) << 1 | color.bit(4, 7) << 2;
  n32 r = color.bit(15) << 0 | color.bit(14) << 1 | color.bit(8,11) << 2;

  n64 R = image::normalize(r, 6, 16);
  n64 G = image::normalize(g, 6, 16);
  n64 B = image::normalize(b, 6, 16);

  if(color.bit(16)) {
    R >>= 1;
    G >>= 1;
    B >>= 1;
  }

  return R << 32 | G << 16 | B << 0;
}
