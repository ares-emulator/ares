auto PPU::Objects::setEnable(n1 status) -> void {
  io.enable[3] = status;
  for(auto& flag : io.enable) flag &= status;
}

auto PPU::Objects::goToNext() -> void {
  if(!(++objIndex)) active = false;
}

auto PPU::Objects::readA01(u32 y) -> void {
  auto& object = latch[1];

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
  n8 ypos           = attr0 >>  0;
  object.affine     = attr0 >>  8;
  object.affineSize = attr0 >>  9;
  object.mode       = attr0 >> 10;
  object.mosaic     = attr0 >> 12;
  object.colors     = attr0 >> 13;  //0 = 16, 1 = 256
  n2 shape          = attr0 >> 14;  //0 = square, 1 = horizontal, 2 = vertical

  n16 attr1 = ppu.oam[objIndex << 2 | 1];
  object.x           = attr1 >>  0;
  object.affineParam = attr1 >>  9;
  object.hflip       = attr1 >> 12;
  object.vflip       = attr1 >> 13;
  n2 size            = attr1 >> 14;

  object.width  = widths [shape * 4 + size];
  object.height = heights[shape * 4 + size];

  //clip left edge if offscreen
  object.px = 0;
  if(object.x >= 240) object.px -= object.x;
  if(!object.affine) object.px &= ~1;

  object.py = y - ypos;
  if((object.affine == 0 && object.affineSize == 1) || (object.py >= object.height << object.affineSize) || (object.px >= object.width << object.affineSize)) {
    //object is hidden - skip rendering
    goToNext();
    state = State::ReadA01;
  } else {
    if(object.mosaic) {
      object.py = ypos >= 160 || mosaicY - ypos >= 0 ? u32(mosaicY - ypos) : 0;
    }
    state = State::ReadA2;
  }
}

auto PPU::Objects::readA2() -> void {
  latch[0] = latch[1];
  auto& object = latch[0];

  n16 attr2 = ppu.oam[objIndex << 2 | 2];
  object.character = attr2 >>  0;
  object.priority  = attr2 >> 10;
  object.palette   = attr2 >> 12;

  if(!object.affine) {
    goToNext();
    state = State::ReadA01;
    vramStageReady = true;
  } else {
    state = State::ReadPA;
  }
}

auto PPU::Objects::drawObject(u32 y) -> void {
  auto& buffer = lineBuffers[y & 1];
  auto& object = latch[0];

  //center-of-sprite coordinates
  i16 centerX = object.width  >> 1;
  i16 centerY = object.height >> 1;

  //current coordinates
  i28 currentX = -(centerX << object.affineSize) + object.px;
  i28 currentY = -(centerY << object.affineSize) + object.py;

  //fractional pixel coordinates
  i28 fx = currentX * object.pa + currentY * object.pb;
  i28 fy = currentX * object.pc + currentY * object.pd;

  //calculate address within tile
  u32 sx, sy;
  if(!object.affine) {
    sx = object.px ^ (object.hflip ? object.width  - 1 : 0);
    sy = object.py ^ (object.vflip ? object.height - 1 : 0);
  } else {
    sx = (fx >> 8) + centerX;
    sy = (fy >> 8) + centerY;
  }
  n6 subTileAddr = ((sy & 7) * 8 + (sx & 7)) >> !object.colors;

  //calculate address of tile
  n10 tileAddr;
  if(io.mapping) {
    u32 offset = (sy >> 3) * (object.width >> 3) + (sx >> 3);
    tileAddr = object.character + (offset << object.colors);
  } else {
    n5 row = (object.character >> 5) + (sy >> 3);
    n5 rowEntry = object.character + ((sx >> 3) << object.colors);
    tileAddr = (row << 5) + rowEntry;
  }

  //output pixel
  n8 color = ppu.readObjectVRAM((tileAddr << 5) + subTileAddr);
  if(object.colors == 0) color = sx & 1 ? color >> 4 : color & 15;
  n9 bx = object.x + object.px;
  if(bx < 240 && sx < object.width && sy < object.height) {
    if(object.mode & 2) {
      if(color) {
        buffer[bx].window = true;
      }
    } else if(!buffer[bx].enable || object.priority < buffer[bx].priority) {
      buffer[bx].priority = object.priority;  //updated regardless of transparency
      buffer[bx].mosaic = object.mosaic;  //updated regardless of transparency
      if(color) {
        if(object.colors == 0) color = object.palette * 16 + color;
        buffer[bx].enable = true;
        buffer[bx].color = 256 + color;
        buffer[bx].translucent = object.mode == 1;
      }
    }
  }

  object.px++;
  if(object.px == (object.width << object.affineSize)) vramStageActive = false;
}

auto PPU::Objects::stepOAM() -> void {
  if(!active) return;
  auto& object = latch[0];

  switch(state) {
  case State::ReadA01:
    readA01(renderY);
    break;
  case State::ReadA2:
    readA2();
    break;
  case State::ReadPA:
    object.pa = ppu.oam[object.affineParam << 4 | 0x3];
    state = State::ReadPB;
    break;
  case State::ReadPB:
    object.pb = ppu.oam[object.affineParam << 4 | 0x7];
    state = State::ReadPC;
    break;
  case State::ReadPC:
    object.pc = ppu.oam[object.affineParam << 4 | 0xb];
    state = State::ReadPD;
    break;
  case State::ReadPD:
    object.pd = ppu.oam[object.affineParam << 4 | 0xf];
    goToNext();
    vramStageReady = true;
    state = State::ReadA01;
    break;
  }
  ppu.oamAccessed = true;
}

auto PPU::Objects::step() -> void {
  auto& object = latch[0];

  if(activeCycle) {
    if(vramStageReady) {
      vramStageReady = false;
      vramStageActive = true;
      if(!object.affine) {
        drawObject(renderY);
        drawObject(renderY);
      }
      stepOAM();
    } else if(vramStageActive) {
      drawObject(renderY);
      if(!object.affine) drawObject(renderY);
      if(!vramStageActive) stepOAM();
    } else {
      stepOAM();
    }
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
  vramStageReady = false;
  vramStageActive = false;
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
  for(auto& object : latch) object = {};
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
