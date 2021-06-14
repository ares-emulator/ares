auto PPU::Window::inside(n8 x, n8 y) const -> bool {
  if((x >= x0 && x <= x1) || (x >= x1 && x <= x0)) {
    if((y >= y0 && y <= y1) || (y >= y1 && y <= y0)) {
      return true;
    }
  }
  return false;
}

auto PPU::Window::outside(n8 x, n8 y) const -> bool {
  if(x < x0 || x > x1 || y < y0 || y > y1) {
    return true;
  }
  return false;
}

auto PPU::Window::within(n8 x, n8 y) const -> bool {
  if(x >= x0 && x <= x1 && y >= y0 && y <= y1) {
    return true;
  }
  return false;
}
