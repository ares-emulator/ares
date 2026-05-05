auto PPU::Objects::setEnable(n1 status) -> void {
  io.enable[3] = status;
  for(auto& flag : io.enable) flag &= status;
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

  for(auto& object : ppu.object) {
    n8 py = y - object.y;
    if(object.affine == 0 && object.affineSize == 1) continue;  //hidden
    if(py >= object.height << object.affineSize) continue;  //offscreen

    if(object.mosaic) {
      py = object.y >= 160 || mosaicY - object.y >= 0 ? u32(mosaicY - object.y) : 0;
    }

    i16 pa = ppu.objectParam[object.affineParam].pa;
    i16 pb = ppu.objectParam[object.affineParam].pb;
    i16 pc = ppu.objectParam[object.affineParam].pc;
    i16 pd = ppu.objectParam[object.affineParam].pd;

    //center-of-sprite coordinates
    i16 centerX = object.width  >> 1;
    i16 centerY = object.height >> 1;

    //origin coordinates (top-left of sprite)
    i28 originX = -(centerX << object.affineSize);
    i28 originY = -(centerY << object.affineSize) + py;

    //fractional pixel coordinates
    i28 fx = originX * pa + originY * pb;
    i28 fy = originX * pc + originY * pd;

    for(u32 px : range(object.width << object.affineSize)) {
      //calculate address within tile
      u32 sx, sy;
      if(!object.affine) {
        sx = px ^ (object.hflip ? object.width  - 1 : 0);
        sy = py ^ (object.vflip ? object.height - 1 : 0);
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
      n9 bx = object.x + px;
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

      fx += pa;
      fy += pc;
    }
  }
}

auto PPU::Objects::outputPixel(u32 x, u32 y) -> void {
  output = {};
  if(ppu.blank() || !io.enable[0]) {
    mosaic = {};
    return;
  }

  auto& buffer = lineBuffers[y & 1];
  output = buffer[x];

  if(hmosaicOffset == io.mosaicWidth) {
    hmosaicOffset = 0;
    mosaic = output;
  } else {
    hmosaicOffset++;
  }

  if(!mosaic.mosaic || !output.mosaic || (output.priority < mosaic.priority)) {
    mosaic = output;
  }
}

auto PPU::Objects::power() -> void {
  io = {};
  for(auto& buffer : lineBuffers) {
    for(auto& pixel : buffer) pixel = {};
  }
  output = {};
  mosaic = {};
  mosaicY = 0;
  hmosaicOffset = 0;
  vmosaicOffset = 0;
}
