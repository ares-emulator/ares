//OB attributes:
//0x80: 0 = OBJ above BG, 1 = BG above OBJ
//0x40: vertical flip
//0x20: horizontal flip
//0x10: palette#

auto PPU::readTileDMG(bool select, uint x, uint y, uint16& tiledata) -> void {
  uint13 tilemapAddress = 0x1800 + (select << 10);
  tilemapAddress += (((y >> 3) << 5) + (x >> 3)) & 0x03ff;

  uint8 tile = vram[tilemapAddress];

  uint13 tiledataAddress;
  if(status.bgTiledataSelect == 0) {
    tiledataAddress = 0x1000 + ( int8(tile) << 4);
  } else {
    tiledataAddress = 0x0000 + (uint8(tile) << 4);
  }

  tiledataAddress += (y & 7) << 1;

  tiledata.byte(0) = vram[tiledataAddress + 0];
  tiledata.byte(1) = vram[tiledataAddress + 1];
}

auto PPU::scanlineDMG() -> void {
  px = 0;

  const int Height = (status.obSize == 0 ? 8 : 16);
  sprites = 0;

  //find first ten sprites on this scanline
  for(uint n = 0; n < 40 * 4; n += 4) {
    Sprite& s = sprite[sprites];
    s.y = oam[n + 0] - 16;
    s.x = oam[n + 1] -  8;
    s.tile = oam[n + 2] & ~status.obSize;
    s.attributes = oam[n + 3];

    if(int(status.ly) <  s.y) continue;
    if(int(status.ly) >= s.y + Height) continue;
    s.y = int(status.ly) - s.y;

    if(s.attributes.bit(6)) s.y ^= Height - 1;
    uint13 tiledataAddress = (s.tile << 4) + (s.y << 1);
    s.tiledata.byte(0) = vram[tiledataAddress + 0];
    s.tiledata.byte(1) = vram[tiledataAddress + 1];
    if(s.attributes.bit(5)) s.tiledata = hflip(s.tiledata);

    if(++sprites == 10) break;
  }

  //sort by X-coordinate
  for(uint lo = 0; lo < sprites; lo++) {
    for(uint hi = lo + 1; hi < sprites; hi++) {
      if(sprite[hi].x < sprite[lo].x) swap(sprite[lo], sprite[hi]);
    }
  }
}

auto PPU::runDMG() -> void {
  bg = {};
  ob = {};

  uint2 color = 0;
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

  auto output = screen->pixels().data() + status.ly * 160 + px++;
  *output = color;
  if(Model::SuperGameBoy()) superGameBoy->ppuWrite(color);
}

auto PPU::runBackgroundDMG() -> void {
  uint8 scrollY = status.ly + status.scy;
  uint8 scrollX = px + status.scx;
  uint3 tileX = scrollX & 7;
  if(tileX == 0 || px == 0) readTileDMG(status.bgTilemapSelect, scrollX, scrollY, background.tiledata);

  uint2 index;
  index.bit(0) = background.tiledata.bit( 7 - tileX);
  index.bit(1) = background.tiledata.bit(15 - tileX);

  bg.color = bgp[index];
  bg.palette = index;
}

auto PPU::runWindowDMG() -> void {
  if(status.ly < status.wy) return;
  if(px + 7 == status.wx) latch.wy++;

  uint8 scrollY = latch.wy - 1;
  uint8 scrollX = px + 7 - latch.wx;
  uint3 tileX = scrollX & 7;

  if(scrollX >= 160u) return;  //also matches underflow (scrollX < 0)
  if(tileX == 0 || px == 0) readTileDMG(status.windowTilemapSelect, scrollX, scrollY, window.tiledata);

  uint2 index;
  index.bit(0) = window.tiledata.bit( 7 - tileX);
  index.bit(1) = window.tiledata.bit(15 - tileX);

  bg.color = bgp[index];
  bg.palette = index;
}

auto PPU::runObjectsDMG() -> void {
  //render backwards, so that first sprite has priority
  for(int n = sprites - 1; n >= 0; n--) {
    Sprite& s = sprite[n];

    int tileX = px - s.x;
    if(tileX < 0 || tileX > 7) continue;

    uint2 index;
    index.bit(0) = s.tiledata.bit( 7 - tileX);
    index.bit(1) = s.tiledata.bit(15 - tileX);
    if(index == 0) continue;
    uint3 palette = s.attributes.bit(4) << 2 | index;

    ob.color = obp[palette];
    ob.palette = index;
    ob.priority = !s.attributes.bit(7);
  }
}
