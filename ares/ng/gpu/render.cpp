auto GPU::render(n9 y) -> void {
  auto output = screen->pixels().data() + y * 320;
  auto backdrop = pram[io.pramBank << 12 | 0xfff];
  for(u32 x : range(320)) {
    output[x] = backdrop;
  }

  auto c1 = 0;
  auto c2 = cartridge.crom.size() >> 2;

  n9 sx = 0;
  n9 sy = 0;
  n6 sh = 0;
  n4 hshrink = ~0;
  n8 vshrink = ~0;

  for(u32 sprite : range(381)) {
    n16 sattributes = vram[0x8000 | sprite];
    n16 yattributes = vram[0x8200 | sprite];
    n16 xattributes = vram[0x8400 | sprite];

    hshrink = sattributes.bit(8,11);  //todo
    if(auto sticky = yattributes.bit(6)) {
      sx += 16;
    } else {
      vshrink = sattributes.bit(0,7);  //todo
      sx = xattributes.bit(7,15);
      sh = yattributes.bit(0, 5);
      sy = yattributes.bit(7,15);
    }
    n9 ry = y - (496 - sy);
    if(sh == 0) continue;
    if(sh >= 33) sh = 32;  //todo: loop borders when shrinking
    if(sx >= 320 && sx + 15 <= 511) continue;
    if(ry >= sh * 16) continue;

    n20 tileNumber = vram[sprite << 6 | ry >> 3 & ~1 | 0];
    n16 attributes = vram[sprite << 6 | ry >> 3 & ~1 | 1];
    n4  hflip      = attributes.bit(0) ? 15 : 0;
    n4  vflip      = attributes.bit(1) ? 15 : 0;
    n1  animation2 = attributes.bit(2);  //todo
    n1  animation3 = attributes.bit(3);  //todo
    n8  palette    = attributes.bit(8,15);
    tileNumber.bit(16,19) = attributes.bit(4,7);
    ry ^= vflip;

    n13 pramAddress = io.pramBank << 12 | palette << 4;
    for(u32 hx = 0; hx < 16; hx += 8) {
      n25 tileAddress = tileNumber << 5 | hx << 1 ^ 16 | ry & 15;
      n16 lower = cartridge.crom[c1 + tileAddress];
      n16 upper = cartridge.crom[c2 + tileAddress];
      for(u32 px : range(8)) {
        n9 rx = sx + (hx + px ^ hflip);
        if(rx >= 320) continue;

        n4 color;
        color.bit(0) = lower.bit(0 + px);
        color.bit(1) = lower.bit(8 + px);
        color.bit(2) = upper.bit(0 + px);
        color.bit(3) = upper.bit(8 + px);
        if(color) {
          output[rx] = pram[pramAddress | color];
        }
      }
    }
  }

  for(u32 x : range(320)) {
    n16 attributes  = vram[0x7000 + (x >> 3) * 0x20 + (y >> 3)];
    n12 tileNumber  = attributes.bit( 0,11);
    n4  palette     = attributes.bit(12,15);
    n13 pramAddress = io.pramBank << 12 | palette << 4;
    n17 tileAddress = tileNumber << 5 | x << 2 & 24 ^ 16 | y & 7;
    n8  tileData    = cartridge.srom[tileAddress];
    n4  color       = tileData >> (x & 1) * 4;
    if(color) {
      output[x] = pram[pramAddress | color];
    }
  }
}
