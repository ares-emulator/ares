auto PPU::Window::scanline(n8 y) -> void {
  enable[0] = enable[1];
  invert[0] = invert[1];
  x0[0] = x0[1];
  x1[0] = x1[1];
  y0[0] = y0[1];
  y1[0] = y1[1];
}

auto PPU::Window::inside(n8 x, n8 y) const -> bool {
  if((x >= x0[0] && x <= x1[0]) || (x >= x1[0] && x <= x0[0])) {
    if((y >= y0[0] && y <= y1[0]) || (y >= y1[0] && y <= y0[0])) {
      return true;
    }
  }
  return false;
}

auto PPU::Window::outside(n8 x, n8 y) const -> bool {
  if(x < x0[0] || x > x1[0] || y < y0[0] || y > y1[0]) {
    return true;
  }
  return false;
}

auto PPU::Window::power() -> void {
  enable[0] = enable[1] = 0;
  invert[0] = invert[1] = 0;
  x0[0] = x0[1] = 0;
  x1[0] = x1[1] = 0;
  y0[0] = y0[1] = 0;
  y1[0] = y1[1] = 0;
}
