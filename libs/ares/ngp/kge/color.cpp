auto KGE::colorNeoGeoPocket(n32 color) -> n64 {
  n3 l = color.bit(0,2);

  n64 L = image::normalize(7 - l, 3, 16);

  return L << 32 | L << 16 | L << 0;
}

auto KGE::colorNeoGeoPocketColor(n32 color) -> n64 {
  n4 r = color.bit(0, 3);
  n4 g = color.bit(4, 7);
  n4 b = color.bit(8,11);

  n64 R = image::normalize(r, 4, 16);
  n64 G = image::normalize(g, 4, 16);
  n64 B = image::normalize(b, 4, 16);

  return R << 32 | G << 16 | B << 0;
}
