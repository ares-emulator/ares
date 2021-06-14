auto PPU::Screen::pixel(n8 x, n8 y) -> void {
  output = {};
  if(!enable) return;

  x += hscroll;
  y += vscroll;

  n15 address = x.bit(3,7) << 1 | y.bit(3,7) << 6 | mapBase << 11;
  n16 attributes = iram.read16(address);

  n10 tile    = attributes.bit( 0, 8) | attributes.bit(13) << 9;
  n4  palette = attributes.bit( 9,12);
  n3  hflip   = attributes.bit(14) * 7;
  n3  vflip   = attributes.bit(15) * 7;

  n4 color = self.fetch(tile & self.tilemask(), x ^ hflip, y ^ vflip);
  if(self.opaque(palette, color)) {
    output.valid = 1;
    output.color = self.palette(palette, color);
  }
}

auto PPU::Screen1::pixel(n8 x, n8 y) -> void {
  Screen::pixel(x, y);
}

auto PPU::Screen2::pixel(n8 x, n8 y) -> void {
  Screen::pixel(x, y);
  if(window.enable) {
    if(window.invert == 0 && window.outside(x, y)) output = {};
    if(window.invert == 1 && window.inside (x, y)) output = {};
  }
}

auto PPU::Screen::power() -> void {
  enable = 0;
  mapBase = 0;
  hscroll = 0;
  vscroll = 0;
  output = {};
}

auto PPU::Screen1::power() -> void {
  Screen::power();
}

auto PPU::Screen2::power() -> void {
  Screen::power();
  window = {};
}
