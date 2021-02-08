auto PPU::Objects::scanline(u32 y) -> void {
  mosaicOffset = 0;
  for(auto& pixel : buffer) pixel = {};
  if(ppu.blank() || !io.enable) return;

  for(auto& object : ppu.object) {
    n8 py = y - object.y;
    if(object.affine == 0 && object.affineSize == 1) continue;  //hidden
    if(py >= object.height << object.affineSize) continue;  //offscreen

    u32 rowSize = io.mapping == 0 ? 32 >> object.colors : object.width >> 3;
    u32 baseAddress = object.character << 5;

    if(object.mosaic && io.mosaicHeight) {
      s32 mosaicY = (y / (1 + io.mosaicHeight)) * (1 + io.mosaicHeight);
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
      u32 sx, sy;
      if(!object.affine) {
        sx = px ^ (object.hflip ? object.width  - 1 : 0);
        sy = py ^ (object.vflip ? object.height - 1 : 0);
      } else {
        sx = (fx >> 8) + centerX;
        sy = (fy >> 8) + centerY;
      }

      n9 bx = object.x + px;
      if(bx < 240 && sx < object.width && sy < object.height) {
        u32 offset = (sy >> 3) * rowSize + (sx >> 3);
        offset = offset * 64 + (sy & 7) * 8 + (sx & 7);

        n8 color = ppu.readObjectVRAM(baseAddress + (offset >> !object.colors));
        if(object.colors == 0) color = sx & 1 ? color >> 4 : color & 15;
        if(object.mode & 2) {
          if(color) {
            buffer[bx].window = true;
          }
        } else if(!buffer[bx].enable || object.priority < buffer[bx].priority) {
          buffer[bx].priority = object.priority;  //updated regardless of transparency
          if(color) {
            if(object.colors == 0) color = object.palette * 16 + color;
            buffer[bx].enable = true;
            buffer[bx].color = ppu.pram[256 + color];
            buffer[bx].translucent = object.mode == 1;
            buffer[bx].mosaic = object.mosaic;
          }
        }
      }

      fx += pa;
      fy += pc;
    }
  }
}

auto PPU::Objects::run(u32 x, u32 y) -> void {
  output = {};
  if(ppu.blank() || !io.enable) {
    mosaic = {};
    return;
  }

  output = buffer[x];

  //horizontal mosaic
  if(!output.mosaic || ++mosaicOffset >= 1 + io.mosaicWidth) {
    mosaicOffset = 0;
    mosaic = output;
  }
}

auto PPU::Objects::power() -> void {
  io = {};
  for(auto& pixel : buffer) pixel = {};
  output = {};
  mosaic = {};
  mosaicOffset = 0;
}
