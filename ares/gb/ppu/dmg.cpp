//OB attributes:
//0x80: 0 = OBJ above BG, 1 = BG above OBJ
//0x40: vertical flip
//0x20: horizontal flip
//0x10: palette#

auto PPU::readTileDMG(bool select, u32 x, u32 y, n16& tiledata) -> void {
  n13 tilemapAddress = 0x1800 + (select << 10);
  tilemapAddress += (((y >> 3) << 5) + (x >> 3)) & 0x03ff;

  n8 tile = vram[tilemapAddress];

  n13 tiledataAddress;
  if(status.bgTiledataSelect == 0) {
    tiledataAddress = 0x1000 + (i8(tile) << 4);
  } else {
    tiledataAddress = 0x0000 + (n8(tile) << 4);
  }

  tiledataAddress += (y & 7) << 1;

  tiledata.byte(0) = vram[tiledataAddress + 0];
  tiledata.byte(1) = vram[tiledataAddress + 1];
}

auto PPU::scanlineDMG() -> void {
  px = 0;

  const s32 Height = (status.obSize == 0 ? 8 : 16);
  sprites = 0;

  //find first ten sprites on this scanline
  for(u32 n = 0; n < 40 * 4; n += 4) {
    Sprite& s = sprite[sprites];
    s.y = oam[n + 0] - 16;
    s.x = oam[n + 1] -  8;
    s.tile = oam[n + 2] & ~status.obSize;
    s.attributes = oam[n + 3];

    if(s32(status.ly) <  s.y) continue;
    if(s32(status.ly) >= s.y + Height) continue;
    s.y = s32(status.ly) - s.y;

    if(s.attributes.bit(6)) s.y ^= Height - 1;
    n13 tiledataAddress = (s.tile << 4) + (s.y << 1);
    s.tiledata.byte(0) = vram[tiledataAddress + 0];
    s.tiledata.byte(1) = vram[tiledataAddress + 1];
    if(s.attributes.bit(5)) s.tiledata = hflip(s.tiledata);

    if(++sprites == 10) break;
  }

  //sort by X-coordinate
  sort(sprite, sprites, [](auto l, auto r) { return l.x < r.x; });
}

auto PPU::runDMG() -> void {
  bg = {};
  ob = {};

  n2 color = 0;
  if(status.bgEnable) runBackgroundDMG();
  if(latch.windowDisplayEnable) runWindowDMG();
  if(status.obEnable) runObjectsDMG();

  if(ob.palette == 0) {
    color = bg.color;
  } else if(bg.palette == 0) {
    color = ob.color;
  } else if(ob.priority) {
    color = ob.color;
  } else {
    color = bg.color;
  }

  if(Model::GameBoy()) {
    auto output = screen->pixels().data() + status.ly * 160 + px++;
    //LCD is still blank during the first frame
    if(!latch.displayEnable) *output = color;
  }
  if(Model::SuperGameBoy()) {
    superGameBoy->ppuWrite(color);
    px++;
  }
}

auto PPU::runBackgroundDMG() -> void {
  n8 scrollY = status.ly + status.scy;
  n8 scrollX = px + status.scx;
  n3 tileX = scrollX & 7;
  if(tileX == 0 || px == 0) readTileDMG(status.bgTilemapSelect, scrollX, scrollY, background.tiledata);

  n2 index;
  index.bit(0) = background.tiledata.bit( 7 - tileX);
  index.bit(1) = background.tiledata.bit(15 - tileX);

  bg.color = bgp[index];
  bg.palette = index;
}

auto PPU::runWindowDMG() -> void {
  if(status.ly < status.wy) return;
  if(px + 7 < status.wx) return;
  if(px + 7 == status.wx) latch.wy++;
  if(!status.bgEnable) return;

  n8 scrollY = latch.wy - 1;
  n8 scrollX = px + 7 - latch.wx;
  n3 tileX = scrollX & 7;

  if(tileX == 0 || px == 0) readTileDMG(status.windowTilemapSelect, scrollX, scrollY, window.tiledata);

  n2 index;
  index.bit(0) = window.tiledata.bit( 7 - tileX);
  index.bit(1) = window.tiledata.bit(15 - tileX);

  bg.color = bgp[index];
  bg.palette = index;
}

auto PPU::runObjectsDMG() -> void {
  //render backwards, so that first sprite has priority
  for(s32 n = sprites - 1; n >= 0; n--) {
    Sprite& s = sprite[n];

    s32 tileX = px - s.x;
    if(tileX < 0 || tileX > 7) continue;

    n2 index;
    index.bit(0) = s.tiledata.bit( 7 - tileX);
    index.bit(1) = s.tiledata.bit(15 - tileX);
    if(index == 0) continue;
    n3 palette = s.attributes.bit(4) << 2 | index;

    ob.color = obp[palette];
    ob.palette = index;
    ob.priority = !s.attributes.bit(7);
  }
}
