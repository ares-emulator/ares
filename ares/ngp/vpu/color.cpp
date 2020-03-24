auto VPU::colorNeoGeoPocket(uint32 color) -> uint64 {
  uint3 l = color.bit(0,2);

  natural L = image::normalize(7 - l, 3, 16);

  return L << 32 | L << 16 | L << 0;
}

auto VPU::colorNeoGeoPocketColor(uint32 color) -> uint64 {
  uint r = color.bit(0, 3);
  uint g = color.bit(4, 7);
  uint b = color.bit(8,11);

  natural R = image::normalize(r, 4, 16);
  natural G = image::normalize(g, 4, 16);
  natural B = image::normalize(b, 4, 16);

  return R << 32 | G << 16 | B << 0;
}
