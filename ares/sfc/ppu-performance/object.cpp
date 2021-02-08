inline auto PPU::Object::addressReset() -> void {
  ppu.io.oamAddress = ppu.io.oamBaseAddress;
  setFirstSprite();
}

inline auto PPU::Object::setFirstSprite() -> void {
  io.firstSprite = 0;
  if(ppu.io.oamPriority) io.firstSprite = ppu.io.oamAddress >> 2;
}

auto PPU::Object::render() -> void {
  if(!io.aboveEnable && !io.belowEnable) return;

  bool windowAbove[256];
  bool windowBelow[256];
  ppu.window.render(window, window.aboveEnable, windowAbove);
  ppu.window.render(window, window.belowEnable, windowBelow);

  u32 itemCount = 0;
  u32 tileCount = 0;
  for(u32 n : range(32)) items[n].valid = false;
  for(u32 n : range(34)) tiles[n].valid = false;

  for(u32 n : range(128)) {
    Item item{true, io.firstSprite + n & 127};
    const auto& object = oam.objects[item.index];
    item.width  = object.width();
    item.height = object.height();

    if(object.x > 256 && object.x + item.width - 1 < 512) continue;
    u32 height = item.height >> io.interlace;
    if((ppu.vcounter() >= object.y && ppu.vcounter() < object.y + height)
    || (object.y + height >= 256 && ppu.vcounter() < (object.y + height & 255))
    ) {
      if(itemCount++ >= 32) break;
      items[itemCount - 1] = item;
    }
  }

  for(s32 n : reverse(range(32))) {
    const auto& item = items[n];
    if(!item.valid) continue;

    const auto& object = oam.objects[item.index];
    u32 tileWidth = item.width >> 3;
    s32 x = object.x;
    s32 y = ppu.vcounter() - object.y & 255;
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
      y = !object.vflip ? y + ppu.field() : y - ppu.field();
    }

    x &= 511;
    y &= 255;

    n16 tiledataAddress = io.tiledataAddress;
    if(object.nameselect) tiledataAddress += 1 + io.nameselect << 12;
    n16 characterX =  object.character.bit(0,3);
    n16 characterY = (object.character.bit(4,7) + (y >> 3) & 15) << 4;

    for(u32 tileX : range(tileWidth)) {
      u32 objectX = x + (tileX << 3) & 511;
      if(x != 256 && objectX >= 256 && objectX + 7 < 512) continue;

      Tile tile{true};
      tile.x = objectX;
      tile.priority = object.priority;
      tile.palette = 128 + (object.palette << 4);
      tile.hflip = object.hflip;

      u32 mirrorX = !object.hflip ? tileX : tileWidth - 1 - tileX;
      u32 address = tiledataAddress + ((characterY + (characterX + mirrorX & 15)) << 4);
      address = (address & 0xfff0) + (y & 7);
      tile.data.bit( 0,15) = ppu.vram[address + 0];
      tile.data.bit(16,31) = ppu.vram[address + 8];

      if(tileCount++ >= 34) break;
      tiles[tileCount - 1] = tile;
    }
  }

  io.rangeOver |= itemCount > 32;
  io.timeOver  |= tileCount > 34;

  n8 palette[256];
  n8 priority[256];

  for(u32 n : range(34)) {
    auto& tile = tiles[n];
    if(!tile.valid) continue;

    u32 tileX = tile.x;
    for(u32 x : range(8)) {
      tileX &= 511;
      if(tileX < 256) {
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

  for(u32 x : range(256)) {
    if(!priority[x]) continue;
    n8 source = palette[x] < 192 ? PPU::Source::OBJ1 : PPU::Source::OBJ2;
    if(io.aboveEnable && !windowAbove[x]) ppu.dac.plotAbove(x, source, priority[x], ppu.dac.cgram[palette[x]]);
    if(io.belowEnable && !windowBelow[x]) ppu.dac.plotBelow(x, source, priority[x], ppu.dac.cgram[palette[x]]);
  }
}

auto PPU::Object::power() -> void {
  for(auto& object : oam.objects) object = {};
  io = {};
  window = {};
}
