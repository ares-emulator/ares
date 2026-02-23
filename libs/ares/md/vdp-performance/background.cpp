auto VDP::Background::renderScreen(u32 from, u32 to) -> void {
  u32 nametableWidth = 32 * (1 + io.nametableWidth);
  u32 nametableWidthMask = nametableWidth - 1;
  u32 nametableHeightMask = 32 * (1 + io.nametableHeight) - 1;

  static const u32 mask[] = {0u, 7u, ~7u, ~0u};
  n15 scrollAddress = io.horizontalScrollAddress;
  scrollAddress += (vdp.state.vcounter & mask[io.horizontalScrollMode]) << 1;
  u32 x = 0 - vdp.vram.memory[scrollAddress + (id == ID::PlaneB)];
  bool interlace = vdp.io.interlaceMode == 3;
  u32 tileShift = interlace ? 7 : 6;

  auto vsram = &vdp.vsram.memory[id == ID::PlaneB];
  u32 y = vdp.state.vcounter;
  if(interlace) y = y << 1 | vdp.state.field;
  y += vdp.vsram.memory[id == ID::PlaneB];
  u32 w = from;
  u32 tileX = x >> 3 & nametableWidthMask;
  u32 tileY = y >> 3 + interlace & nametableHeightMask;
  u32 tileY_x_width = tileY * nametableWidth;
  u32 maskY = interlace ? 15 : 7;
  u32 address = io.nametableAddress + (tileY_x_width + tileX & 0x0fff);
  u32 tileAttributes = vdp.vram.memory[address & 0x7fff];
  u32 flipX = tileAttributes & 0x0800 ? 7 : 0;
  u32 flipY = tileAttributes & 0x1000 ? maskY : 0;
  u32 pixelY = y & maskY ^ flipY;
  auto tileData = &vdp.vram.pixels[(tileAttributes << tileShift) + (pixelY << 3) + (x & 7 ^ flipX) & 0x1ffff];
  s32 incrementX = flipX ? -1 : +1;

  while(w < to) {
    pixels[w].color = *tileData ? tileAttributes >> 9 & 0x30 | *tileData : 0;
    pixels[w].priority = tileAttributes >> 15;
    tileData += incrementX;

    if((w++ & 15) == 15 && io.verticalScrollMode) {
      y = vdp.state.vcounter;
      if(interlace) y = y << 1 | vdp.state.field;
      y += vsram[w >> 4 << 1];
      if((x++ & 7) == 7) tileX = x >> 3 & nametableWidthMask;
      tileY = y >> 3 + interlace & nametableHeightMask;
      tileY_x_width = tileY * nametableWidth;
      pixelY = y & maskY;
      address = io.nametableAddress + (tileY_x_width + tileX & 0x0fff);
      tileAttributes = vdp.vram.memory[address & 0x7fff];
      flipX = tileAttributes & 0x0800 ? 7 : 0;
      flipY = tileAttributes & 0x1000 ? maskY : 0;
      pixelY = y & maskY ^ flipY;
      tileData = &vdp.vram.pixels[(tileAttributes << tileShift) + (pixelY << 3) + (x & 7 ^ flipX) & 0x1ffff];
      incrementX = flipX ? -1 : +1;
    }

    else if((x++ & 7) == 7) {
      tileX = x >> 3 & nametableWidthMask;
      address = io.nametableAddress + (tileY_x_width + tileX & 0x0fff);
      tileAttributes = vdp.vram.memory[address & 0x7fff];
      flipX = tileAttributes & 0x0800 ? 7 : 0;
      flipY = tileAttributes & 0x1000 ? maskY : 0;
      pixelY = y & maskY ^ flipY;
      tileData = &vdp.vram.pixels[(tileAttributes << tileShift) + (pixelY << 3) + (x & 7 ^ flipX) & 0x1ffff];
      incrementX = flipX ? -1 : +1;
    }
  }
}

auto VDP::Background::renderWindow(u32 from, u32 to) -> void {
  bool interlace = vdp.io.interlaceMode == 3;
  u32 tileShift = interlace ? 7 : 6;

  u32 y = vdp.state.vcounter;
  if(interlace) y = y << 1 | vdp.state.field;

  u32 nametableAddress = io.nametableAddress & (vdp.io.displayWidth ? ~0x0400 : ~0);
  u32 widthSize = 32 << (bool)vdp.io.displayWidth;
  u32 widthMask = widthSize - 1;

  u32 x = from;
  u32 tileX = x >> 3 & widthMask;
  u32 tileY = y >> 3 + interlace & 31;
  u32 tileY_x_width = tileY * widthSize;
  u32 maskY = interlace ? 15 : 7;
  u32 address = nametableAddress + (tileY_x_width + tileX & 0x0fff);
  u32 tileAttributes = vdp.vram.memory[address & 0x7fff];
  u32 flipX = tileAttributes & 0x0800 ? 7 : 0;
  u32 flipY = tileAttributes & 0x1000 ? maskY : 0;
  u32 pixelY = y & maskY ^ flipY;
  auto tileData = &vdp.vram.pixels[(tileAttributes << tileShift) + (pixelY << 3) + (x & 7 ^ flipX) & 0x1ffff];
  s32 incrementX = flipX ? -1 : +1;

  while(x < to) {
    vdp.planeA.pixels[x].color = *tileData ? tileAttributes >> 9 & 0x30 | *tileData : 0;
    vdp.planeA.pixels[x].priority = tileAttributes >> 15;
    tileData += incrementX;

    if((x++ & 7) == 7) {
      tileX = x >> 3 & widthMask;
      address = nametableAddress + (tileY_x_width + tileX & 0x0fff);
      tileAttributes = vdp.vram.memory[address & 0x7fff];
      flipX = tileAttributes & 0x0800 ? 7 : 0;
      flipY = tileAttributes & 0x1000 ? maskY : 0;
      pixelY = y & maskY ^ flipY;
      tileData = &vdp.vram.pixels[(tileAttributes << tileShift) + (pixelY << 3) + (x & 7 ^ flipX) & 0x1ffff];
      incrementX = flipX ? -1 : +1;
    }
  }
}
