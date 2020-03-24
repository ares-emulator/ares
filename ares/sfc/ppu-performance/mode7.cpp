auto PPU::Background::renderMode7() -> void {
  int Y = ppu.vcounter();
  if(io.mosaicEnable) Y -= ppu.mosaic.voffset();  //BG2 vertical mosaic uses BG1 mosaic enable
  int y = !ppu.mode7.vflip ? Y : 255 - Y;

  int a = (int16)ppu.mode7.a;
  int b = (int16)ppu.mode7.b;
  int c = (int16)ppu.mode7.c;
  int d = (int16)ppu.mode7.d;
  int hcenter = (int13)ppu.mode7.hcenter;
  int vcenter = (int13)ppu.mode7.vcenter;
  int hoffset = (int13)ppu.mode7.hoffset;
  int voffset = (int13)ppu.mode7.voffset;

  uint mosaicCounter = 1;
  uint mosaicPalette = 0;
  uint8 mosaicPriority = 0;
  uint15 mosaicColor = 0;

  auto clip = [](int n) -> int { return n & 0x2000 ? (n | ~1023) : (n & 1023); };
  int originX = (a * clip(hoffset - hcenter) & ~63) + (b * clip(voffset - vcenter) & ~63) + (b * y & ~63) + (hcenter << 8);
  int originY = (c * clip(hoffset - hcenter) & ~63) + (d * clip(voffset - vcenter) & ~63) + (d * y & ~63) + (vcenter << 8);

  bool windowAbove[256];
  bool windowBelow[256];
  ppu.window.render(window, window.aboveEnable, windowAbove);
  ppu.window.render(window, window.belowEnable, windowBelow);

  for(int X : range(256)) {
    int x = !ppu.mode7.hflip ? X : 255 - X;

    int pixelX = originX + a * x >> 8;
    int pixelY = originY + c * x >> 8;

    uint7 tileX = pixelX >> 3;
    uint7 tileY = pixelY >> 3;

    bool outOfBounds = (pixelX | pixelY) & ~1023;

    uint16 tileAddress = tileY << 7 | tileX;
    uint16 paletteAddress = (uint3)pixelY << 3 | (uint3)pixelX;

    uint8 tile = ppu.mode7.repeat == 3 && outOfBounds ? 0 : ppu.vram[tileAddress] >> 0;
    uint8 palette = ppu.mode7.repeat == 2 && outOfBounds ? 0 : ppu.vram[tile << 6 | paletteAddress] >> 8;

    uint8 priority;
    if(id == ID::BG1) {
      priority = io.priority[0];
    } else {
      priority = io.priority[palette.bit(7)];
      palette.bit(7) = 0;
    }

    if(--mosaicCounter == 0) {
      mosaicCounter = io.mosaicEnable ? (uint)ppu.mosaic.size : 1;
      mosaicPalette = palette;
      mosaicPriority = priority;
      if(ppu.dac.io.directColor && id == ID::BG1) {
        mosaicColor = ppu.dac.directColor(0, palette);
      } else {
        mosaicColor = ppu.dac.cgram[palette];
      }
    }
    if(!mosaicPalette) continue;

    if(io.aboveEnable && !windowAbove[X]) ppu.dac.plotAbove(X, id, mosaicPriority, mosaicColor);
    if(io.belowEnable && !windowBelow[X]) ppu.dac.plotBelow(X, id, mosaicPriority, mosaicColor);
  }
}
