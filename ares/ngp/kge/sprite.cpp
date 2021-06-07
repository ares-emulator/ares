//technically, the Neo Geo Pocket attempts to buffer a line of sprites in advance.
//so long as the user program does not write to VRAM during display, it can render
//all 64 sprites on the same line. currently, the "character over" condition when
//writing to VRAM during active display is not emulated, nor is the exact timing
//of the caching used by real hardware, as this information is not currently known.

auto KGE::Sprite::begin(n8 y) -> void {
  for(auto& tile : tiles) tile.priority = 0;

  n8 count;
  n8 px;
  n8 py;
  for(auto& object : objects) {
    n8 ox = object.hoffset;
    n8 oy = object.voffset;
    if(object.hchain) ox += px;
    if(object.vchain) oy += py;
    px = ox;
    py = oy;
    if(!object.priority) continue;  //invisible

    ox = (ox + hscroll);
    oy = y - (oy + vscroll);

    if(oy >= 8) continue;  //out of range
    if(object.vflip) oy ^= 7;

    auto& tile = tiles[count++];
    tile.x = ox;
    tile.y = oy;
    tile.character = object.character;
    tile.priority  = object.priority;
    tile.palette   = object.palette;
    tile.hflip     = object.hflip;
    tile.code      = object.code;
  }
}

auto KGE::Sprite::render(n8 x, n8 y) -> maybe<Output&> {
  for(auto& tile : tiles) {
    if(!tile.priority) continue;  //invisible

    n8 tx = x - tile.x;
    n3 ty = tile.y;

    if(tx >= 8) continue;  //out of range
    if(tile.hflip) tx ^= 7;

    if(n2 index = self.characters[tile.character][ty][tx]) {
      output.priority = tile.priority;
      if(Model::NeoGeoPocket()) {
        output.color = palette[tile.palette][index];
      }
      if(Model::NeoGeoPocketColor()) {
        switch(self.dac.colorMode) {
        case 0: output.color = tile.code    * 4 + index; break;
        case 1: output.color = tile.palette * 8 + palette[tile.palette][index]; break;
        }
      }
      return output;
    }
  }

  return {};
}

auto KGE::Sprite::power() -> void {
  output = {};
  for(auto& object : objects) object = {};
  for(auto& tile : tiles) tile = {};
  hscroll = 0;
  vscroll = 0;
  memory::assign(palette[0], 0, 0, 0, 0);
  memory::assign(palette[1], 0, 0, 0, 0);
}
