auto PPU::Objects::setEnable(n1 status) -> void {
  io.enable[3] = status;
  for(auto& flag : io.enable) flag &= status;
}

auto PPU::Objects::scanline(u32 y) -> void {
  if(y >= 160) return;

  static const u32 widths[] = {
     8, 16, 32, 64,
    16, 32, 32, 64,
     8,  8, 16, 32,
     8,  8,  8,  8,  //invalid modes
  };

  static const u32 heights[] = {
     8, 16, 32, 64,
     8,  8, 16, 32,
    16, 32, 32, 64,
     8,  8,  8,  8,  //invalid modes
  };

  hmosaicOffset = io.mosaicWidth;
  if(y == 0 || vmosaicOffset == io.mosaicHeight) {
    vmosaicOffset = 0;
    mosaicY = y;
  } else {
    vmosaicOffset++;
  }

  auto& buffer = lineBuffers[y & 1];
  for(auto& pixel : buffer) pixel = {};
  if(ppu.io.forceBlank[1] || cpu.stopped() || !io.enable[1]) return;  //checks if display conditions will be met next scanline

  for(auto oamIndex : range(128)) {
    n16 attr0 = ppu.oam[oamIndex << 2 | 0];
    n8 ypos       = attr0 >>  0;
    n1 affine     = attr0 >>  8;
    n1 affineSize = attr0 >>  9;
    n2 mode       = attr0 >> 10;
    n1 mosaic     = attr0 >> 12;
    n1 colors     = attr0 >> 13;  //0 = 16, 1 = 256
    n2 shape      = attr0 >> 14;  //0 = square, 1 = horizontal, 2 = vertical

    n16 attr1 = ppu.oam[oamIndex << 2 | 1];
    n9 xpos        = attr1 >>  0;
    n5 affineParam = attr1 >>  9;
    n1 hflip       = attr1 >> 12;
    n1 vflip       = attr1 >> 13;
    n2 size        = attr1 >> 14;

    n8 py = y - ypos;
    n32 width  = widths [shape * 4 + size];
    n32 height = heights[shape * 4 + size];
    if(affine == 0 && affineSize == 1) continue;  //hidden
    if(py >= height << affineSize) continue;  //offscreen

    if(mosaic) {
      py = ypos >= 160 || mosaicY - ypos >= 0 ? u32(mosaicY - ypos) : 0;
    }

    n16 attr2 = ppu.oam[oamIndex << 2 | 2];
    n10 character = attr2 >>  0;
    n2  priority  = attr2 >> 10;
    n4  palette   = attr2 >> 12;

    i16 pa = ppu.oam[affineParam << 4 | 0x3];
    i16 pb = ppu.oam[affineParam << 4 | 0x7];
    i16 pc = ppu.oam[affineParam << 4 | 0xb];
    i16 pd = ppu.oam[affineParam << 4 | 0xf];

    //center-of-sprite coordinates
    i16 centerX = width  >> 1;
    i16 centerY = height >> 1;

    //origin coordinates (top-left of sprite)
    i28 originX = -(centerX << affineSize);
    i28 originY = -(centerY << affineSize) + py;

    //fractional pixel coordinates
    i28 fx = originX * pa + originY * pb;
    i28 fy = originX * pc + originY * pd;

    for(u32 px : range(width << affineSize)) {
      //calculate address within tile
      u32 sx, sy;
      if(!affine) {
        sx = px ^ (hflip ? width  - 1 : 0);
        sy = py ^ (vflip ? height - 1 : 0);
      } else {
        sx = (fx >> 8) + centerX;
        sy = (fy >> 8) + centerY;
      }
      n6 subTileAddr = ((sy & 7) * 8 + (sx & 7)) >> !colors;

      //calculate address of tile
      n10 tileAddr;
      if(io.mapping) {
        u32 offset = (sy >> 3) * (width >> 3) + (sx >> 3);
        tileAddr = character + (offset << colors);
      } else {
        n5 row = (character >> 5) + (sy >> 3);
        n5 rowEntry = character + ((sx >> 3) << colors);
        tileAddr = (row << 5) + rowEntry;
      }

      //output pixel
      n8 color = ppu.readObjectVRAM((tileAddr << 5) + subTileAddr);
      if(colors == 0) color = sx & 1 ? color >> 4 : color & 15;
      n9 bx = xpos + px;
      if(bx < 240 && sx < width && sy < height) {
        if(mode & 2) {
          if(color) {
            buffer[bx].window = true;
          }
        } else if(!buffer[bx].enable || priority < buffer[bx].priority) {
          buffer[bx].priority = priority;  //updated regardless of transparency
          buffer[bx].mosaic = mosaic;  //updated regardless of transparency
          if(color) {
            if(colors == 0) color = palette * 16 + color;
            buffer[bx].enable = true;
            buffer[bx].color = 256 + color;
            buffer[bx].translucent = mode == 1;
          }
        }
      }

      fx += pa;
      fy += pc;
    }
  }
}

auto PPU::Objects::outputPixel(u32 x, u32 y) -> void {
  output = {};
  if(ppu.blank() || !io.enable[0]) {
    mosaicLatch = {};
    return;
  }

  auto& buffer = lineBuffers[y & 1];
  output = buffer[x];

  if(hmosaicOffset == io.mosaicWidth) {
    hmosaicOffset = 0;
    mosaicLatch = output;
  } else {
    hmosaicOffset++;
  }

  if(!mosaicLatch.mosaic || !output.mosaic || (output.priority < mosaicLatch.priority)) {
    mosaicLatch = output;
  }
}

auto PPU::Objects::power() -> void {
  io = {};
  for(auto& buffer : lineBuffers) {
    for(auto& pixel : buffer) pixel = {};
  }
  output = {};
  mosaicLatch = {};
  mosaicY = 0;
  hmosaicOffset = 0;
  vmosaicOffset = 0;
}
