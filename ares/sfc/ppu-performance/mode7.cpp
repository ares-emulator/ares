auto PPU::Background::renderMode7() -> void {
  s32 Y = self.vcounter();
  if(io.mosaicEnable) Y -= self.mosaic.voffset();  //BG2 vertical mosaic uses BG1 mosaic enable
  s32 y = !self.mode7.vflip ? Y : 255 - Y;

  s32 a = (i16)self.mode7.a;
  s32 b = (i16)self.mode7.b;
  s32 c = (i16)self.mode7.c;
  s32 d = (i16)self.mode7.d;
  s32 hcenter = (i13)self.mode7.hcenter;
  s32 vcenter = (i13)self.mode7.vcenter;
  s32 hoffset = (i13)self.mode7.hoffset;
  s32 voffset = (i13)self.mode7.voffset;

  u32 mosaicCounter = 1;
  u32 mosaicPalette = 0;
  n8  mosaicPriority = 0;
  n15 mosaicColor = 0;

  auto clip = [](s32 n) -> s32 { return n & 0x2000 ? (n | ~1023) : (n & 1023); };
  s32 originX = (a * clip(hoffset - hcenter) & ~63) + (b * clip(voffset - vcenter) & ~63) + (b * y & ~63) + (hcenter << 8);
  s32 originY = (c * clip(hoffset - hcenter) & ~63) + (d * clip(voffset - vcenter) & ~63) + (d * y & ~63) + (vcenter << 8);

  bool windowAbove[448];
  bool windowBelow[448];
  self.window.render(window, window.aboveEnable, windowAbove);
  self.window.render(window, window.belowEnable, windowBelow);

  s32 x1 =   0;
  s32 x2 = 255;
  if(self.width() == 352) x1 = -48, x2 = 303;
  if(self.width() == 448) x1 = -96, x2 = 351;

  for(s32 X = x1; X <= x2; X++) {
    s32 x = !self.mode7.hflip ? X : x2 - X;

    s32 pixelX = originX + a * x >> 8;
    s32 pixelY = originY + c * x >> 8;

    n7 tileX = pixelX >> 3;
    n7 tileY = pixelY >> 3;

    bool outOfBounds = (pixelX | pixelY) & ~1023;

    n16 tileAddress = tileY << 7 | tileX;
    n16 paletteAddress = (n3)pixelY << 3 | (n3)pixelX;

    n8 tile = self.mode7.repeat == 3 && outOfBounds ? 0 : self.vram[tileAddress] >> 0;
    n8 palette = self.mode7.repeat == 2 && outOfBounds ? 0 : self.vram[tile << 6 | paletteAddress] >> 8;

    n8 priority;
    if(id == ID::BG1) {
      priority = io.priority[0];
    } else {
      priority = io.priority[palette.bit(7)];
      palette.bit(7) = 0;
    }

    if(--mosaicCounter == 0) {
      mosaicCounter = io.mosaicEnable ? (u32)self.mosaic.size : 1;
      mosaicPalette = palette;
      mosaicPriority = priority;
      if(self.dac.io.directColor && id == ID::BG1) {
        mosaicColor = self.dac.directColor(palette, 0);
      } else {
        mosaicColor = self.dac.cgram[palette];
      }
    }
    if(!mosaicPalette) continue;

    u32 Xp = X + abs(x1);
    if(io.aboveEnable && !windowAbove[Xp]) self.dac.plotAbove(Xp, id, mosaicPriority, mosaicColor);
    if(io.belowEnable && !windowBelow[Xp]) self.dac.plotBelow(Xp, id, mosaicPriority, mosaicColor);
  }
}
