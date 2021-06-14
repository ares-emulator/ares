inline auto PPU::Object::addressReset() -> void {
  self.io.oamAddress = self.io.oamBaseAddress;
  setFirstSprite();
}

inline auto PPU::Object::setFirstSprite() -> void {
  io.firstSprite = 0;
  if(self.io.oamPriority) io.firstSprite = self.io.oamAddress >> 2;
}

auto PPU::Object::render() -> void {
  if(!io.aboveEnable && !io.belowEnable) return;

  bool windowAbove[448];
  bool windowBelow[448];
  self.window.render(window, window.aboveEnable, windowAbove);
  self.window.render(window, window.belowEnable, windowBelow);

  u32 itemCount = 0;
  u32 tileCount = 0;
  for(u32 n : range(32)) items[n].valid = false;
  for(u32 n : range(34)) tiles[n].valid = false;

  u32 width = self.width();
  s32 x1 = 0, x2 = 255;
  if(self.width() == 352) x1 = -48, x2 = 303;
  if(self.width() == 448) x1 = -96, x2 = 351;

  for(u32 n : range(128)) {
    Item item{true, io.firstSprite + n & 127};
    const auto& object = oam.objects[item.index];
    item.width  = object.width();
    item.height = object.height();

    if(within<-128,+383>(object.x, item.width, x1, x2)) {
      u32 height = item.height >> io.interlace;
      if(auto y = within<0,511>(object.y, height, self.vcounter())) {
        item.y = y();
        if(itemCount++ >= 32) break;
        items[itemCount - 1] = item;
      }
    }
  }

  for(s32 n : reverse(range(32))) {
    const auto& item = items[n];
    if(!item.valid) continue;

    const auto& object = oam.objects[item.index];
    u32 tileWidth = item.width >> 3;
    s32 x = object.x;
    s32 y = item.y;
    if(io.interlace) y <<= 1;

    if(object.vflip) {
      if(item.width == item.height) {
        y = item.height - 1 - y;
      } else if(y < item.width) {
        y = item.width - 1 - y;
      } else {
        y = item.width + (item.width - 1) - (y - item.width);
      }
    }

    if(io.interlace) {
      y = !object.vflip ? y + self.field() : y - self.field();
    }

    x &= 511;
    y &= 255;

    n16 tiledataAddress = io.tiledataAddress;
    if(object.nameselect) tiledataAddress += 1 + io.nameselect << 12;
    n16 characterX =  object.character.bit(0,3);
    n16 characterY = (object.character.bit(4,7) + (y >> 3) & 15) << 4;

    for(u32 tileX : range(tileWidth)) {
      u32 objectX = x + (tileX << 3) & 511;
      if(x != width && objectX >= width && objectX + 7 < 512 + x1) continue;

      Tile tile{true};
      tile.x = objectX;
      tile.priority = object.priority;
      tile.palette = 128 + (object.palette << 4);
      tile.hflip = object.hflip;

      u32 mirrorX = !object.hflip ? tileX : tileWidth - 1 - tileX;
      u32 address = tiledataAddress + ((characterY + (characterX + mirrorX & 15)) << 4);
      address = (address & 0xfff0) + (y & 7);
      tile.data.bit( 0,15) = self.vram[address + 0];
      tile.data.bit(16,31) = self.vram[address + 8];

      if(tileCount++ >= 34) break;
      tiles[tileCount - 1] = tile;
    }
  }

  io.rangeOver |= itemCount > 32;
  io.timeOver  |= tileCount > 34;

  n8 palette[448];
  n8 priority[448];

  for(u32 n : range(34)) {
    auto& tile = tiles[n];
    if(!tile.valid) continue;

    u32 tileX = tile.x + abs(x1);
    for(u32 x : range(8)) {
      tileX &= 511;
      if(tileX < width) {
        u32 color = 0, shift = tile.hflip ? x : 7 - x;
        color += tile.data >> shift +  0 & 1;
        color += tile.data >> shift +  7 & 2;
        color += tile.data >> shift + 14 & 4;
        color += tile.data >> shift + 21 & 8;
        if(color) {
          palette[tileX] = tile.palette + color;
          priority[tileX] = io.priority[tile.priority];
        }
      }
      tileX++;
    }
  }

  for(s32 x = x1; x <= x2; x++) {
    u32 xp = x + abs(x1);
    if(!priority[xp]) continue;
    n8 source = palette[xp] < 192 ? PPU::Source::OBJ1 : PPU::Source::OBJ2;
    if(io.aboveEnable && !windowAbove[xp]) self.dac.plotAbove(xp, source, priority[xp], self.dac.cgram[palette[xp]]);
    if(io.belowEnable && !windowBelow[xp]) self.dac.plotBelow(xp, source, priority[xp], self.dac.cgram[palette[xp]]);
  }
}

auto PPU::Object::power() -> void {
  for(auto& object : oam.objects) object = {};
  io = {};
  window = {};
}
