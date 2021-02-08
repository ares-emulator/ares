auto PPU::Background::runMode7() -> void {
  s32 a = (i16)ppu.io.m7a;
  s32 b = (i16)ppu.io.m7b;
  s32 c = (i16)ppu.io.m7c;
  s32 d = (i16)ppu.io.m7d;

  s32 hcenter = (i13)ppu.io.m7x;
  s32 vcenter = (i13)ppu.io.m7y;
  s32 hoffset = (i13)ppu.io.hoffsetMode7;
  s32 voffset = (i13)ppu.io.voffsetMode7;

  u32 x = mosaic.hoffset;
  u32 y = ppu.vcounter();
  if(ppu.bg1.mosaic.enable) y -= ppu.mosaic.voffset();  //BG2 vertical mosaic uses BG1 mosaic enable

  if(!mosaic.enable) {
    mosaic.hoffset += 1;
  } else if(--mosaic.hcounter == 0) {
    mosaic.hcounter = ppu.mosaic.size;
    mosaic.hoffset += ppu.mosaic.size;
  }

  if(ppu.io.hflipMode7) x = 255 - x;
  if(ppu.io.vflipMode7) y = 255 - y;

  //13-bit sign extend: --s---nnnnnnnnnn -> ssssssnnnnnnnnnn
  auto clip = [](s32 n) -> s32 { return n & 0x2000 ? (n | ~1023) : (n & 1023); };
  s32 originX = (a * clip(hoffset - hcenter) & ~63) + (b * clip(voffset - vcenter) & ~63) + (b * y & ~63) + (hcenter << 8);
  s32 originY = (c * clip(hoffset - hcenter) & ~63) + (d * clip(voffset - vcenter) & ~63) + (d * y & ~63) + (vcenter << 8);

  s32 pixelX = originX + a * x >> 8;
  s32 pixelY = originY + c * x >> 8;
  n16 paletteAddress = (n3)pixelY << 3 | (n3)pixelX;

  n7  tileX = pixelX >> 3;
  n7  tileY = pixelY >> 3;
  n16 tileAddress = tileY << 7 | tileX;

  bool outOfBounds = (pixelX | pixelY) & ~1023;

  n8 tile = ppu.io.repeatMode7 == 3 && outOfBounds ? 0 : ppu.vram[tileAddress] >> 0;
  n8 palette = ppu.io.repeatMode7 == 2 && outOfBounds ? 0 : ppu.vram[tile << 6 | paletteAddress] >> 8;

  u32 priority;
  if(id == ID::BG1) {
    priority = io.priority[0];
  } else if(id == ID::BG2) {
    priority = io.priority[palette.bit(7)];
    palette.bit(7) = 0;
  }

  if(palette == 0) return;

  if(io.aboveEnable) {
    output.above.priority = priority;
    output.above.palette = palette;
    output.above.paletteGroup = 0;
  }

  if(io.belowEnable) {
    output.below.priority = priority;
    output.below.palette = palette;
    output.below.paletteGroup = 0;
  }
}
