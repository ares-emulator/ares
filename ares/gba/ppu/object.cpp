auto PPU::Objects::setEnable(n1 status) -> void {
  io.enable[3] = status;
  for(auto& flag : io.enable) flag &= status;
}

auto PPU::Objects::goToNext() -> void {
  if(!(++objIndex)) active = false;
  state = State::ReadA01;
}

auto PPU::Objects::readA01(u32 y) -> void {
  static const u32 widths[] = {
     8, 16, 32, 64,
    16, 32, 32, 64,
     8,  8, 16, 32,
     8,  8,  8,  8,  //invalid shapes
  };

  static const u32 heights[] = {
     8, 16, 32, 64,
     8,  8, 16, 32,
    16, 32, 32, 64,
     8,  8,  8,  8,  //invalid shapes
  };

  n16 attr0 = ppu.oam[objIndex << 2 | 0];
  n8 ypos          = attr0 >>  0;
  latch.affine     = attr0 >>  8;
  latch.affineSize = attr0 >>  9;
  latch.mode       = attr0 >> 10;
  latch.mosaic     = attr0 >> 12;
  latch.colors     = attr0 >> 13;  //0 = 16, 1 = 256
  n2 shape         = attr0 >> 14;  //0 = square, 1 = horizontal, 2 = vertical

  n16 attr1 = ppu.oam[objIndex << 2 | 1];
  latch.x           = attr1 >>  0;
  latch.affineParam = attr1 >>  9;
  latch.hflip       = attr1 >> 12;
  latch.vflip       = attr1 >> 13;
  n2 size           = attr1 >> 14;

  latch.width  = widths [shape * 4 + size];
  latch.height = heights[shape * 4 + size];

  latch.py = y - ypos;
  n9 pxMax = latch.x + (latch.width << latch.affineSize);
  if((latch.affine == 0 && latch.affineSize == 1) || (latch.py >= latch.height << latch.affineSize) || (latch.x >= 240 && pxMax >= 240)) {
    //object is hidden - skip rendering
    goToNext();
  } else {
    if(latch.mosaic) {
      latch.py = ypos >= 160 || mosaicY - ypos >= 0 ? u32(mosaicY - ypos) : 0;
    }
    state = State::ReadA2;
  }
}

auto PPU::Objects::readA2() -> void {
  n16 attr2 = ppu.oam[objIndex << 2 | 2];
  latch.character = attr2 >>  0;
  latch.priority  = attr2 >> 10;
  latch.palette   = attr2 >> 12;

  if(!latch.affine) {
    drawObject(renderY);
  } else {
    state = State::ReadPA;
  }
}

auto PPU::Objects::drawObject(u32 y) -> void {
  auto& buffer = lineBuffers[y & 1];

  //center-of-sprite coordinates
  i16 centerX = latch.width  >> 1;
  i16 centerY = latch.height >> 1;

  //origin coordinates (top-left of sprite)
  i28 originX = -(centerX << latch.affineSize);
  i28 originY = -(centerY << latch.affineSize) + latch.py;

  //fractional pixel coordinates
  i28 fx = originX * latch.pa + originY * latch.pb;
  i28 fy = originX * latch.pc + originY * latch.pd;

  for(u32 px : range(latch.width << latch.affineSize)) {
    //calculate address within tile
    u32 sx, sy;
    if(!latch.affine) {
      sx =       px ^ (latch.hflip ? latch.width  - 1 : 0);
      sy = latch.py ^ (latch.vflip ? latch.height - 1 : 0);
    } else {
      sx = (fx >> 8) + centerX;
      sy = (fy >> 8) + centerY;
    }
    n6 subTileAddr = ((sy & 7) * 8 + (sx & 7)) >> !latch.colors;

    //calculate address of tile
    n10 tileAddr;
    if(io.mapping) {
      u32 offset = (sy >> 3) * (latch.width >> 3) + (sx >> 3);
      tileAddr = latch.character + (offset << latch.colors);
    } else {
      n5 row = (latch.character >> 5) + (sy >> 3);
      n5 rowEntry = latch.character + ((sx >> 3) << latch.colors);
      tileAddr = (row << 5) + rowEntry;
    }

    //output pixel
    n8 color = ppu.readObjectVRAM((tileAddr << 5) + subTileAddr);
    if(latch.colors == 0) color = sx & 1 ? color >> 4 : color & 15;
    n9 bx = latch.x + px;
    if(bx < 240 && sx < latch.width && sy < latch.height) {
      if(latch.mode & 2) {
        if(color) {
          buffer[bx].window = true;
        }
      } else if(!buffer[bx].enable || latch.priority < buffer[bx].priority) {
        buffer[bx].priority = latch.priority;  //updated regardless of transparency
        buffer[bx].mosaic = latch.mosaic;  //updated regardless of transparency
        if(color) {
          if(latch.colors == 0) color = latch.palette * 16 + color;
          buffer[bx].enable = true;
          buffer[bx].color = 256 + color;
          buffer[bx].translucent = latch.mode == 1;
        }
      }
    }

    fx += latch.pa;
    fy += latch.pc;
  }

  goToNext();
}

auto PPU::Objects::step() -> void {
  if(!active) return;

  if(activeCycle) {
    switch(state) {
    case State::ReadA01:
      readA01(renderY);
      break;
    case State::ReadA2:
      readA2();
      break;
    case State::ReadPA:
      latch.pa = ppu.oam[latch.affineParam << 4 | 0x3];
      state = State::ReadPB;
      break;
    case State::ReadPB:
      latch.pb = ppu.oam[latch.affineParam << 4 | 0x7];
      state = State::ReadPC;
      break;
    case State::ReadPC:
      latch.pc = ppu.oam[latch.affineParam << 4 | 0xb];
      state = State::ReadPD;
      break;
    case State::ReadPD:
      latch.pd = ppu.oam[latch.affineParam << 4 | 0xf];
      drawObject(renderY);
      break;
    }
    ppu.oamAccessed = true;
  }
  activeCycle = !activeCycle;
}

auto PPU::Objects::scanline(u32 y) -> void {
  if(y >= 160) return;

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

  renderY = y;
  objIndex = 0;
  active = true;
  activeCycle = true;
  state = State::ReadA01;
}

auto PPU::Objects::renderScanline(u32 y) -> void {
  scanline(y);
  for(auto _ : range(1232)) step();
  active = false;
  ppu.objReleaseBus();
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
  latch = {};
  for(auto& buffer : lineBuffers) {
    for(auto& pixel : buffer) pixel = {};
  }
  output = {};
  mosaicLatch = {};
  renderY = 0;
  mosaicY = 0;
  hmosaicOffset = 0;
  vmosaicOffset = 0;
  objIndex = 0;
  active = false;
  activeCycle = false;
  state = State::ReadA01;
}
