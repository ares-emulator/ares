auto VDP::colorMasterSystem(n32 color) -> n64 {
  n2 B = color >> 4;
  n2 G = color >> 2;
  n2 R = color >> 0;

  n64 r = image::normalize(R, 2, 16);
  n64 g = image::normalize(G, 2, 16);
  n64 b = image::normalize(B, 2, 16);

  return r << 32 | g << 16 | b << 0;
}

auto VDP::colorGameGear(n32 color) -> n64 {
  n4 B = color >> 8;
  n4 G = color >> 4;
  n4 R = color >> 0;

  n64 r = image::normalize(R, 4, 16);
  n64 g = image::normalize(G, 4, 16);
  n64 b = image::normalize(B, 4, 16);

  return r << 32 | g << 16 | b << 0;
}
