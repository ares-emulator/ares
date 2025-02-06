auto LSPC::render(n9 y) -> void {
  auto output = screen->pixels().data() + y * 320;
  auto backdrop = io.shadow << 16 | pram[io.pramBank << 12 | 0xfff];
  for(u32 x : range(320)) {
    output[x] = backdrop;
  }

  n9 sx = 0;
  n9 sy = 0;
  n6 sh = 0;
  n4 hshrink = ~0;
  n8 vshrink = ~0;

  for(u32 sprite : range(381)) {
    n16 sattributes = vram[0x8000 | sprite];
    n16 yattributes = vram[0x8200 | sprite];
    n16 xattributes = vram[0x8400 | sprite];

    if(auto sticky = yattributes.bit(6)) {
      sx += hshrink + 1;
    } else {
      vshrink = sattributes.bit(0,7);
      sx = xattributes.bit(7,15);
      sh = yattributes.bit(0, 5);
      sy = yattributes.bit(7,15);
    }
    hshrink = sattributes.bit(8,11);

    n9 ry = y - (496 + 16 - sy);
    if(sh == 0) continue;
    if(sh >= 33) sh = 32;  //todo: loop borders when shrinking
    if(sx >= 320 && sx + 15 <= 511) continue;
    if(ry >= sh * 16) continue;

    switch(ry >> 8) {
    case 0: ry = vscale[vshrink][n8( ry)] ^ 0x000; break;
    case 1: ry = vscale[vshrink][n8(~ry)] ^ 0x1ff; break;
    }

    n20 tileNumber = vram[sprite << 6 | ry >> 3 & ~1 | 0];
    n16 attributes = vram[sprite << 6 | ry >> 3 & ~1 | 1];
    n4  hflip      = attributes.bit(0) ? 15 : 0;
    n1  vflip      = attributes.bit(1);
    n2  animate    = attributes.bit(2,3);
    n8  palette    = attributes.bit(8,15);

    tileNumber.bit(16,19) = attributes.bit(4,7);
    if(vflip) ry ^= sh * 16 - 1;
    switch(animate * !animation.disable) {
    case 0: break;
    case 1: tileNumber.bit(0,1) = animation.frame.bit(0,1); break;
    case 2: tileNumber.bit(0,2) = animation.frame.bit(0,2); break;
    case 3: tileNumber.bit(0,2) = animation.frame.bit(0,2); break;
    }

    n13 pramAddress = io.pramBank << 12 | palette << 4;
    n27 tileAddress = (tileNumber << 5 | ry & 15) << 2;

    n16 d0 = system.readC(tileAddress + 0) << 8 | system.readC(tileAddress + 64 + 0) << 0;
    n16 d1 = system.readC(tileAddress + 2) << 8 | system.readC(tileAddress + 64 + 2) << 0;
    n16 d2 = system.readC(tileAddress + 1) << 8 | system.readC(tileAddress + 64 + 1) << 0;
    n16 d3 = system.readC(tileAddress + 3) << 8 | system.readC(tileAddress + 64 + 3) << 0;

    n9  px = 0;
    n4  bx = hflip;
    for(u32 x : range(16)) {
      if(hscale[hshrink][x]) {
        n9 rx = sx + px++;
        if (rx >= 320) continue;

        n4 color;
        color.bit(0) = d0.bit(bx);
        color.bit(1) = d1.bit(bx);
        color.bit(2) = d2.bit(bx);
        color.bit(3) = d3.bit(bx);
        if (color) {
          output[rx] = io.shadow << 16 | pram[pramAddress | color];
        }
      }
      bx += hflip ? -1 : 1;
    }
  }

  for(u32 x : range(320)) {
    n16 attributes  = vram[0x7000 + (x >> 3) * 0x20 + (y >> 3)];
    n12 tileNumber  = attributes.bit( 0,11);
    n4  palette     = attributes.bit(12,15);
    n13 pramAddress = io.pramBank << 12 | palette << 4;
    n17 tileAddress = tileNumber << 5 | x << 2 & 24 ^ 16 | y & 7;
    n8  tileData    = system.readS(tileAddress);
    n4  color       = tileData >> (x & 1) * 4;
    if(color) {
      output[x] = io.shadow << 16 | pram[pramAddress | color];
    }
  }
}
