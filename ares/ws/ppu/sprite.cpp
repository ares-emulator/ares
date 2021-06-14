auto PPU::Sprite::frame() -> void {
  n7 index = first;
  n16 base = oamBase.bit(0, self.depth() == 2 ? 4 : 5) << 9;
  for(auto object : range(min(128, count))) {
    oam[self.field()][object] = iram.read32(base + index++ * 4);
  }
}

auto PPU::Sprite::scanline(n8 y) -> void {
  valid = 0;
  if(!enable) return;
  for(auto object : range(min(128, count))) {
    n32 attributes = oam[!self.field()][object];
    n8 voffset = attributes.bit(16,23);
    if(n8(y - voffset) > 7) continue;
    objects[valid++] = attributes;
    if(valid == 32) break;
  }
}

auto PPU::Sprite::pixel(n8 x, n8 y) -> void {
  output = {};
  if(!enable) return;
  auto within = window.within(x, y);
  for(auto index : range(valid)) {
    auto attributes = objects[index];
    n9 tile     = attributes.bit( 0, 8);
    n4 palette  = attributes.bit( 9,11) | 8;
    n1 region   = attributes.bit(12);
    n1 priority = attributes.bit(13);
    n3 hflip    = attributes.bit(14) * 7;
    n3 vflip    = attributes.bit(15) * 7;
    n8 voffset  = attributes.bit(16,23);
    n8 hoffset  = attributes.bit(24,31);

    if(window.enable && region == within) continue;
    if(n8(x - hoffset) > 7) continue;

    n4 color = self.fetch(tile, x - hoffset ^ hflip, y - voffset ^ vflip);
    if(self.opaque(palette, color)) {
      if(!priority && screen2.output.valid) continue;
      output.valid = 1;
      output.color = self.palette(palette, color);
      break;
    }
  }
}

auto PPU::Sprite::power() -> void {
  enable = 0;
  oamBase = 0;
  first = 0;
  count = 0;
  valid = 0;
  output = {};
  window = {};
  for(auto& line : oam) for(auto& attributes : line) attributes = 0;
  for(auto& object : objects) object = 0;
}
