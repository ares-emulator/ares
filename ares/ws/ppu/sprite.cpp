auto PPU::Sprite::frame() -> void {
  n7 index = first;
  n16 base = oamBase.bit(0, self.depth() == 2 ? 4 : 5) << 9;
  oam[self.field()].flush();
  for(auto object : range(min(128, count))) {
    oam[self.field()].write(iram.read32(base + index++ * 4));
  }
}

auto PPU::Sprite::scanline(n8 y) -> void {
  enable[0] = enable[1];
  window.scanline(y);
  objects.flush();
  if(!enable[0]) return;
  for(auto attributes : oam[!self.field()]) {
    n8 voffset = attributes.bit(16,23);
    if(n8(y - voffset) > 7) continue;
    objects.write(attributes);
    if(objects.full()) break;
  }
}

auto PPU::Sprite::pixel(n8 x, n8 y) -> void {
  output = {};
  if(!enable[0]) return;
  auto outside = window.outside(x, y);
  for(auto attributes : objects) {
    n9 tile     = attributes.bit( 0, 8);
    n4 palette  = attributes.bit( 9,11) | 8;
    n1 region   = attributes.bit(12);
    n1 priority = attributes.bit(13);
    n3 hflip    = attributes.bit(14) * 7;
    n3 vflip    = attributes.bit(15) * 7;
    n8 voffset  = attributes.bit(16,23);
    n8 hoffset  = attributes.bit(24,31);

    if(window.enable[0] && region != outside) continue;
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
  enable[0] = enable[1] = 0;
  oamBase = 0;
  first = 0;
  count = 0;
  valid = 0;
  output = {};
  window.power();
  for(auto& line : oam) line.flush();
  objects.flush();
}
