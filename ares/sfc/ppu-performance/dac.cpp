auto PPU::DAC::prepare() -> void {
  bool hires = ppu.io.pseudoHires || ppu.io.bgMode == 5 || ppu.io.bgMode == 6;

  uint15 aboveColor = cgram[0];
  uint15 belowColor = hires ? cgram[0] : fixedColor();
  if(ppu.io.displayDisable) aboveColor = 0, belowColor = 0;

  for(uint x : range(256)) {
    above[x] = {PPU::Source::COL, 0, aboveColor};
    below[x] = {PPU::Source::COL, 0, belowColor};
  }
}

auto PPU::DAC::render() -> void {
  ppu.window.render(window, window.aboveMask, windowAbove);
  ppu.window.render(window, window.belowMask, windowBelow);

  auto output = ppu.output + ppu.vcounter() * 1024 + (ppu.interlace() && ppu.field() ? 512 : 0);

  uint luma = ppu.io.displayBrightness << 15;
  if(!ppu.hires()) {
    for(uint x : range(256)) {
      *output++ = luma | pixel(x, above[x], below[x]);
    }
  } else {
    for(uint x : range(256)) {
      *output++ = luma | pixel(x, below[x], above[x]);
      *output++ = luma | pixel(x, above[x], below[x]);
    }
  }
}

auto PPU::DAC::pixel(uint8 x, Pixel above, Pixel below) const -> uint15 {
  if(!windowAbove[x]) above.color = 0x0000;
  if(!windowBelow[x]) return above.color;
  if(!io.colorEnable[above.source]) return above.color;
  if(!io.blendMode) return blend(above.color, fixedColor(), io.colorHalve && windowAbove[x]);
  return blend(above.color, below.color, io.colorHalve && windowAbove[x] && below.source != PPU::Source::COL);
}

auto PPU::DAC::blend(uint15 x, uint15 y, bool halve) const -> uint15 {
  if(!io.colorMode) {
    if(!halve) {
      uint sum = x + y;
      uint carry = (sum - ((x ^ y) & 0x0421)) & 0x8420;
      return (sum - carry) | (carry - (carry >> 5));
    } else {
      return (x + y - ((x ^ y) & 0x0421)) >> 1;
    }
  } else {
    uint diff = x - y + 0x8420;
    uint borrow = (diff - ((x ^ y) & 0x8420)) & 0x8420;
    if(!halve) {
      return   (diff - borrow) & (borrow - (borrow >> 5));
    } else {
      return (((diff - borrow) & (borrow - (borrow >> 5))) & 0x7bde) >> 1;
    }
  }
}

inline auto PPU::DAC::plotAbove(uint8 x, uint8 source, uint8 priority, uint15 color) -> void {
  if(priority > above[x].priority) above[x] = {source, priority, color};
}

inline auto PPU::DAC::plotBelow(uint8 x, uint8 source, uint8 priority, uint15 color) -> void {
  if(priority > below[x].priority) below[x] = {source, priority, color};
}

inline auto PPU::DAC::directColor(uint8 palette, uint3 paletteGroup) const -> uint15 {
  //palette = -------- BBGGGRRR
  //group   = -------- -----bgr
  //output  = 0BBb00GG Gg0RRRr0
  return (palette << 7 & 0x6000) + (paletteGroup << 10 & 0x1000)
       + (palette << 4 & 0x0380) + (paletteGroup <<  5 & 0x0040)
       + (palette << 2 & 0x001c) + (paletteGroup <<  1 & 0x0002);
}

inline auto PPU::DAC::fixedColor() const -> uint15 {
  return io.colorRed << 0 | io.colorGreen << 5 | io.colorBlue << 10;
}

auto PPU::DAC::power() -> void {
  for(auto& color : cgram) color = 0;
  io = {};
  window = {};
}
