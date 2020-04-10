//technically, the Neo Geo Pocket attempts to buffer a line of sprites in advance.
//so long as the user program does not write to VRAM during display, it can render
//all 64 sprites on the same line. currently, the "character over" condition when
//writing to VRAM during active display is not emulated, nor is the exact timing
//of the caching used by real hardware, as this information is not currently known.

auto VPU::cacheSprites(uint8 ly) -> void {
  tileCount = 0;

  uint8 px = 0;
  uint8 py = 0;
  for(auto& s : sprites) {
    uint8 sx = s.hoffset;
    uint8 sy = s.voffset;
    if(s.hchain) sx += px;
    if(s.vchain) sy += py;
    px = sx;
    py = sy;
    if(s.priority == 0) continue;

    uint8 x = sx + sprite.hscroll + 7;
    uint8 y = sy + sprite.vscroll + 7 - ly;
    if(y >= 8) continue;  //out of range?
    if(s.vflip == 0) y ^= 7;

    tiles[tileCount++] = {x, y, s.character, s.priority, s.palette, s.hflip, s.code};
  }
}

auto VPU::renderSprite(uint8 lx) -> bool {
  for(uint tileIndex : range(tileCount)) {
    auto& t = tiles[tileIndex];

    uint8 x = t.x - lx;
    uint8 y = t.y;
    if(x >= 8) continue;  //out of range?
    if(t.hflip == 1) x ^= 7;  //bit endian of tiledata is reversed

    if(uint2 index = characters[t.character][y][x]) {
      sprite.priority = t.priority;
      if(Model::NeoGeoPocket()) {
        sprite.output = sprite.palette[t.palette][index];
      }
      if(Model::NeoGeoPocketColor()) {
        if(dac.colorMode) {
          sprite.output = colors[sprite.colorCompatible + t.palette * 8 + sprite.palette[t.palette][index]];
        } else {
          sprite.output = colors[sprite.colorNative + t.code * 4 + index];
        }
      }
      return true;
    }
  }

  return false;
}
