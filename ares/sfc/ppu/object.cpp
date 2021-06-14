inline auto PPU::Object::addressReset() -> void {
  self.io.oamAddress = self.io.oamBaseAddress;
  setFirstSprite();
}

inline auto PPU::Object::setFirstSprite() -> void {
  io.firstSprite = 0;
  if(self.io.oamPriority) io.firstSprite = self.io.oamAddress >> 2;
}

auto PPU::Object::frame() -> void {
  io.timeOver = false;
  io.rangeOver = false;
}

auto PPU::Object::scanline() -> void {
  latch.firstSprite = io.firstSprite;

  t.x = 0;
  t.y = self.vcounter();
  t.itemCount = 0;
  t.tileCount = 0;

  t.active = !t.active;
  auto oamItem = t.item[t.active];
  auto oamTile = t.tile[t.active];

  for(u32 n : range(32)) oamItem[n].valid = false;
  for(u32 n : range(34)) oamTile[n].valid = false;

  if(t.y == self.vdisp() && !self.io.displayDisable) addressReset();
  if(t.y >= self.vdisp() - 1 || self.io.displayDisable) return;
}

auto PPU::Object::evaluate(n7 index) -> void {
  if(self.io.displayDisable) return;
  if(t.itemCount > 32) return;

  auto oamItem = t.item[t.active];
  auto oamTile = t.tile[t.active];

  n7 sprite = latch.firstSprite + index;
  if(!onScanline(oam.object[sprite])) return;
  self.latch.oamAddress = sprite;

  if(t.itemCount++ < 32) {
    oamItem[t.itemCount - 1] = {true, sprite};
  }
}

auto PPU::Object::onScanline(PPU::OAM::Object& sprite) -> bool {
  if(sprite.x > 256 && sprite.x + sprite.width() - 1 < 512) return false;
  u32 height = sprite.height() >> io.interlace;
  return (bool)within<0,511>(sprite.y, height, t.y);
}

auto PPU::Object::run() -> void {
  output.above.priority = 0;
  output.below.priority = 0;

  auto oamTile = t.tile[!t.active];
  u32 x = t.x++;

  for(u32 n : range(34)) {
    const auto& tile = oamTile[n];
    if(!tile.valid) break;

    s32 px = x - (i9)tile.x;
    if(px & ~7) continue;

    u32 color = 0, shift = tile.hflip ? px : 7 - px;
    color += tile.data >> shift +  0 & 1;
    color += tile.data >> shift +  7 & 2;
    color += tile.data >> shift + 14 & 4;
    color += tile.data >> shift + 21 & 8;

    if(color) {
      if(io.aboveEnable) {
        output.above.palette = tile.palette + color;
        output.above.priority = io.priority[tile.priority];
      }

      if(io.belowEnable) {
        output.below.palette = tile.palette + color;
        output.below.priority = io.priority[tile.priority];
      }
    }
  }
}

auto PPU::Object::fetch() -> void {
  auto oamItem = t.item[t.active];
  auto oamTile = t.tile[t.active];

  for(u32 i : reverse(range(32))) {
    if(!oamItem[i].valid) continue;

    if(self.io.displayDisable || self.vcounter() >= self.vdisp() - 1) {
      self.step(8);
      continue;
    }

    self.latch.oamAddress = oamItem[i].index;
    const auto& sprite = oam.object[self.latch.oamAddress];

    u32 tileWidth = sprite.width() >> 3;
    s32 x = sprite.x;
    s32 y = t.y - sprite.y & 255;
    if(io.interlace) y <<= 1;

    if(sprite.vflip) {
      if(sprite.width() == sprite.height()) {
        y = sprite.height() - 1 - y;
      } else if(y < sprite.width()) {
        y = sprite.width() - 1 - y;
      } else {
        y = sprite.width() + (sprite.width() - 1) - (y - sprite.width());
      }
    }

    if(io.interlace) {
      y = !sprite.vflip ? y + self.field() : y - self.field();
    }

    x &= 511;
    y &= 255;

    n16 tiledataAddress = io.tiledataAddress;
    if(sprite.nameselect) tiledataAddress += 1 + io.nameselect << 12;
    n16 chrx =  sprite.character.bit(0,3);
    n16 chry = (sprite.character.bit(4,7) + (y >> 3) & 15) << 4;

    for(u32 tx : range(tileWidth)) {
      u32 sx = x + (tx << 3) & 511;
      if(x != 256 && sx >= 256 && sx + 7 < 512) continue;
      if(t.tileCount++ >= 34) break;

      u32 n = t.tileCount - 1;
      oamTile[n].valid = true;
      oamTile[n].x = sx;
      oamTile[n].priority = sprite.priority;
      oamTile[n].palette = 128 + (sprite.palette << 4);
      oamTile[n].hflip = sprite.hflip;

      u32 mx = !sprite.hflip ? tx : tileWidth - 1 - tx;
      u32 pos = tiledataAddress + ((chry + (chrx + mx & 15)) << 4);
      n16 address = (pos & 0xfff0) + (y & 7);

      if(!self.io.displayDisable)
      oamTile[n].data.bit( 0,15) = vram[address + 0];
      self.step(4);

      if(!self.io.displayDisable)
      oamTile[n].data.bit(16,31) = vram[address + 8];
      self.step(4);
    }
  }

  io.rangeOver |= t.itemCount > 32;
  io.timeOver  |= t.tileCount > 34;
}

auto PPU::Object::power() -> void {
  t.x = 0;
  t.y = 0;

  t.itemCount = 0;
  t.tileCount = 0;

  t.active = 0;
  for(u32 p : range(2)) {
    for(auto& item : t.item[p]) {
      item.valid = false;
      item.index = 0;
    }
    for(auto& tile : t.tile[p]) {
      tile.valid = false;
      tile.x = 0;
      tile.priority = 0;
      tile.palette = 0;
      tile.hflip = 0;
      tile.data = 0;
    }
  }

  io.aboveEnable = random();
  io.belowEnable = random();
  io.interlace = random();

  io.baseSize = random();
  io.nameselect = random();
  io.tiledataAddress = (random() & 7) << 13;
  io.firstSprite = 0;

  for(auto& p : io.priority) p = 0;

  io.timeOver = false;
  io.rangeOver = false;

  latch = {};

  output.above.palette = 0;
  output.above.priority = 0;
  output.below.palette = 0;
  output.below.priority = 0;
}
