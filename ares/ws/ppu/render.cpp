auto PPU::renderFetch(n10 tile, n3 x, n3 y) -> n4 {
  n4 color;

  if(planar() && depth() == 2) {
    n16 data = iram.read16(0x2000 + (tile << 4) + (y << 1));
    color.bit(0) = data.bit( 7 - x);
    color.bit(1) = data.bit(15 - x);
  }

  if(planar() && depth() == 4) {
    n32 data = iram.read32(0x4000 + (tile << 5) + (y << 2));
    color.bit(0) = data.bit( 7 - x);
    color.bit(1) = data.bit(15 - x);
    color.bit(2) = data.bit(23 - x);
    color.bit(3) = data.bit(31 - x);
  }

  if(packed() && depth() == 2) {
    n8 data = iram.read8(0x2000 + (tile << 4) + (y << 1) + (x >> 2));
    color.bit(0,1) = data >> 6 - (x.bit(0,1) << 1);
  }

  if(packed() && depth() == 4) {
    n8 data = iram.read8(0x4000 + (tile << 5) + (y << 2) + (x >> 1));
    color.bit(0,3) = data >> 4 - (x.bit(0) << 2);
  }

  return color;
}

auto PPU::renderTransparent(bool palette, n4 color) -> bool {
  if(color) return false;
  if(depth() == 2 && !palette) return false;
  return true;
}

auto PPU::renderPalette(n4 palette, n4 color) -> n12 {
  if(grayscale()) {
    n3 paletteColor = r.palette[palette].color[color.bit(0,1)];
    n4 poolColor = 15 - r.pool[paletteColor];
    return poolColor << 0 | poolColor << 4 | poolColor << 8;
  } else {
    return iram.read16(0xfe00 + (palette << 5) + (color << 1));
  }
}

auto PPU::renderBack() -> void {
  if(grayscale()) {
    n4 poolColor = 15 - r.pool[l.backColor.bit(0,2)];
    s.pixel = {Pixel::Source::Back, poolColor << 0 | poolColor << 4 | poolColor << 8};
  } else {
    n12 color = iram.read16(0xfe00 + (l.backColor << 1));
    s.pixel = {Pixel::Source::Back, color};
  }
}

auto PPU::renderScreenOne(n8 x, n8 y) -> void {
  n8 scrollX = x + l.scrollOneX;
  n8 scrollY = y + l.scrollOneY;

  n15 tilemapOffset = 0;
  tilemapOffset.bit( 1, 5) = scrollX >> 3;
  tilemapOffset.bit( 6,10) = scrollY >> 3;
  tilemapOffset.bit(11,14) = l.screenOneMapBase.bit(0, depth() == 2 ? 2 : 3);

  n16 attributes = iram.read16(tilemapOffset);
  n10 tile = attributes.bit(13) << 9 | attributes.bit(0,8);
  n3  tileX = scrollX ^ attributes.bit(14) * 7;
  n3  tileY = scrollY ^ attributes.bit(15) * 7;
  n4  tileColor = renderFetch(tile & tilemask(), tileX, tileY);
  if(renderTransparent(attributes.bit(11), tileColor)) return;

  s.pixel = {Pixel::Source::ScreenOne, renderPalette(attributes.bit(9,12), tileColor)};
}

auto PPU::renderScreenTwo(n8 x, n8 y) -> void {
  auto x0 = l.screenTwoWindowX0;
  auto x1 = l.screenTwoWindowX1;
  if(x0 > x1) swap(x0, x1);

  auto y0 = l.screenTwoWindowY0;
  auto y1 = l.screenTwoWindowY1;
  if(y0 > y1) swap(y0, y1);

  bool windowInside = x >= x0 && x <= x1 && y >= y0 && y <= y1;
  windowInside ^= l.screenTwoWindowInvert;
  if(l.screenTwoWindowEnable && !windowInside) return;

  n8 scrollX = x + l.scrollTwoX;
  n8 scrollY = y + l.scrollTwoY;

  n15 tilemapOffset = 0;
  tilemapOffset.bit( 1, 5) = scrollX >> 3;
  tilemapOffset.bit( 6,10) = scrollY >> 3;
  tilemapOffset.bit(11,14) = l.screenTwoMapBase.bit(0, depth() == 2 ? 2 : 3);

  n16 attributes = iram.read16(tilemapOffset);
  n10 tile = attributes.bit(13) << 9 | attributes.bit(0,8);
  n3  tileX = scrollX ^ attributes.bit(14) * 7;
  n3  tileY = scrollY ^ attributes.bit(15) * 7;
  n4  tileColor = renderFetch(tile & tilemask(), tileX, tileY);
  if(renderTransparent(attributes.bit(11), tileColor)) return;

  s.pixel = {Pixel::Source::ScreenTwo, renderPalette(attributes.bit(9,12), tileColor)};
}

auto PPU::renderSprite(n8 x, n8 y) -> void {
  auto x0 = l.spriteWindowX0;
  auto x1 = l.spriteWindowX1;
  if(x0 > x1) swap(x0, x1);

  auto y0 = l.spriteWindowY0;
  auto y1 = l.spriteWindowY1;
  if(y0 > y1) swap(y0, y1);

  bool windowInside = x >= x0 && x <= x1 && y >= y0 && y <= y1;
  for(auto index : range(l.spriteCount)) {
    auto sprite = l.sprite[index];
    if(l.spriteWindowEnable && sprite.bit(12) == windowInside) continue;
    if((n8)(x - sprite.bit(24,31)) > 7) continue;

    n3 tileX = (x - sprite.bit(24,31)) ^ sprite.bit(14) * 7;
    n3 tileY = (y - sprite.bit(16,23)) ^ sprite.bit(15) * 7;
    n4 tileColor = renderFetch(sprite.bit(0,8), tileX, tileY);
    if(renderTransparent(sprite.bit(11), tileColor)) continue;
    if(!sprite.bit(13) && s.pixel.source == Pixel::Source::ScreenTwo) continue;

    s.pixel = {Pixel::Source::Sprite, renderPalette(8 + sprite.bit(9,11), tileColor)};
    break;
  }
}
