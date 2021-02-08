auto PPU::Background::renderMode7() -> void {
  s32 Y = ppu.vcounter();
  if(io.mosaicEnable) Y -= ppu.mosaic.voffset();  //BG2 vertical mosaic uses BG1 mosaic enable
  s32 y = !ppu.mode7.vflip ? Y : 255 - Y;

  s32 a = (i16)ppu.mode7.a;
  s32 b = (i16)ppu.mode7.b;
  s32 c = (i16)ppu.mode7.c;
  s32 d = (i16)ppu.mode7.d;
  s32 hcenter = (i13)ppu.mode7.hcenter;
  s32 vcenter = (i13)ppu.mode7.vcenter;
  s32 hoffset = (i13)ppu.mode7.hoffset;
  s32 voffset = (i13)ppu.mode7.voffset;

  u32 mosaicCounter = 1;
  u32 mosaicPalette = 0;
  n8  mosaicPriority = 0;
  n15 mosaicColor = 0;

  auto clip = [](s32 n) -> s32 { return n & 0x2000 ? (n | ~1023) : (n & 1023); };
  s32 originX = (a * clip(hoffset - hcenter) & ~63) + (b * clip(voffset - vcenter) & ~63) + (b * y & ~63) + (hcenter << 8);
  s32 originY = (c * clip(hoffset - hcenter) & ~63) + (d * clip(voffset - vcenter) & ~63) + (d * y & ~63) + (vcenter << 8);

  bool windowAbove[256];
  bool windowBelow[256];
  ppu.window.render(window, window.aboveEnable, windowAbove);
  ppu.window.render(window, window.belowEnable, windowBelow);

  for(s32 X : range(256)) {
    s32 x = !ppu.mode7.hflip ? X : 255 - X;

    s32 pixelX = originX + a * x >> 8;
    s32 pixelY = originY + c * x >> 8;

    n7 tileX = pixelX >> 3;
    n7 tileY = pixelY >> 3;

    bool outOfBounds = (pixelX | pixelY) & ~1023;

    n16 tileAddress = tileY << 7 | tileX;
    n16 paletteAddress = (n3)pixelY << 3 | (n3)pixelX;

    n8 tile = ppu.mode7.repeat == 3 && outOfBounds ? 0 : ppu.vram[tileAddress] >> 0;
    n8 palette = ppu.mode7.repeat == 2 && outOfBounds ? 0 : ppu.vram[tile << 6 | paletteAddress] >> 8;

    n8 priority;
    if(id == ID::BG1) {
      priority = io.priority[0];
    } else {
      priority = io.priority[palette.bit(7)];
      palette.bit(7) = 0;
    }

    if(--mosaicCounter == 0) {
      mosaicCounter = io.mosaicEnable ? (u32)ppu.mosaic.size : 1;
      mosaicPalette = palette;
      mosaicPriority = priority;
      if(ppu.dac.io.directColor && id == ID::BG1) {
        mosaicColor = ppu.dac.directColor(palette, 0);
      } else {
        mosaicColor = ppu.dac.cgram[palette];
      }
    }
    if(!mosaicPalette) continue;

    if(io.aboveEnable && !windowAbove[X]) ppu.dac.plotAbove(X, id, mosaicPriority, mosaicColor);
    if(io.belowEnable && !windowBelow[X]) ppu.dac.plotBelow(X, id, mosaicPriority, mosaicColor);
  }
}
