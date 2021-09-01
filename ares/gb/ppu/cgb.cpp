//BG attributes:
//0x80: 0 = OAM priority, 1 = BG priority
//0x40: vertical flip
//0x20: horizontal flip
//0x08: VRAM bank#
//0x07: palette#

//OB attributes:
//0x80: 0 = OBJ above BG, 1 = BG above OBJ
//0x40: vertical flip
//0x20: horizontal flip
//0x08: VRAM bank#
//0x07: palette#

auto PPU::readTileCGB(bool select, u32 x, u32 y, n16& tiledata, n8& attributes) -> void {
  n14 tilemapAddress = 0x1800 + (select << 10);
  tilemapAddress += (((y >> 3) << 5) + (x >> 3)) & 0x03ff;

  n8 tile = vram[0x0000 + tilemapAddress];
  attributes = vram[0x2000 + tilemapAddress];

  n14 tiledataAddress = attributes.bit(3) ? 0x2000 : 0x0000;
  if(status.bgTiledataSelect == 0) {
    tiledataAddress += 0x1000 + (i8(tile) << 4);
  } else {
    tiledataAddress += 0x0000 + (n8(tile) << 4);
  }

  if(attributes.bit(6)) y ^= 7;
  tiledataAddress += (y & 7) << 1;

  tiledata.byte(0) = vram[tiledataAddress + 0];
  tiledata.byte(1) = vram[tiledataAddress + 1];
  if(attributes.bit(5)) tiledata = hflip(tiledata);
}

auto PPU::scanlineCGB() -> void {
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
    s.y = status.ly - s.y;

    if(s.attributes.bit(6)) s.y ^= Height - 1;
    n14 tiledataAddress = (s.attributes.bit(3) ? 0x2000 : 0x0000) + (s.tile << 4) + (s.y << 1);
    s.tiledata.byte(0) = vram[tiledataAddress + 0];
    s.tiledata.byte(1) = vram[tiledataAddress + 1];
    if(s.attributes.bit(5)) s.tiledata = hflip(s.tiledata);

    if(++sprites == 10) break;
  }
}

auto PPU::runCGB() -> void {
  bg = {};
  ob = {};

  n15 color = 0x7fff;
  runBackgroundCGB();
  if(latch.windowDisplayEnable) runWindowCGB();
  if(status.obEnable) runObjectsCGB();

  if(ob.palette == 0) {
    color = bg.color;
  } else if(bg.palette == 0) {
    color = ob.color;
  } else if(status.bgEnable == 0) {
    color = ob.color;
  } else if(bg.priority) {
    color = bg.color;
  } else if(ob.priority) {
    color = ob.color;
  } else {
    color = bg.color;
  }

  if(Model::GameBoyColor()) {
    auto output = screen->pixels().data() + status.ly * 160 + px++;
    *output = color;
  }
}

auto PPU::runBackgroundCGB() -> void {
  n8 scrollY = status.ly + status.scy;
  n8 scrollX = px + status.scx;
  n3 tileX = scrollX & 7;
  if(tileX == 0 || px == 0) readTileCGB(status.bgTilemapSelect, scrollX, scrollY, background.tiledata, background.attributes);

  n2 index;
  index.bit(0) = background.tiledata.bit( 7 - tileX);
  index.bit(1) = background.tiledata.bit(15 - tileX);
  n5 palette = background.attributes.bit(0,2) << 2 | index;

  bg.color = bgpd[palette];
  bg.palette = index;
  bg.priority = background.attributes.bit(7);
}

auto PPU::runWindowCGB() -> void {
  if(status.ly < status.wy) return;
  if(px + 7 < status.wx) return;
  if(px + 7 == status.wx) latch.wy++;

  n8 scrollY = latch.wy - 1;
  n8 scrollX = px + 7 - latch.wx;
  n3 tileX = scrollX & 7;

  if(tileX == 0 || px == 0) readTileCGB(status.windowTilemapSelect, scrollX, scrollY, window.tiledata, window.attributes);

  n2 index;
  index.bit(0) = window.tiledata.bit( 7 - tileX);
  index.bit(1) = window.tiledata.bit(15 - tileX);
  n5 palette = window.attributes.bit(0,2) << 2 | index;

  bg.color = bgpd[palette];
  bg.palette = index;
  bg.priority = window.attributes.bit(7);
}

auto PPU::runObjectsCGB() -> void {
  //render backwards, so that first sprite has priority
  for(s32 n = sprites - 1; n >= 0; n--) {
    Sprite& s = sprite[n];

    s32 tileX = px - s.x;
    if(tileX < 0 || tileX > 7) continue;

    n2 index;
    index.bit(0) = s.tiledata.bit( 7 - tileX);
    index.bit(1) = s.tiledata.bit(15 - tileX);
    if(index == 0) continue;
    n5 palette = s.attributes.bit(0,2) << 2 | index;

    ob.color = obpd[palette];
    ob.palette = index;
    ob.priority = !s.attributes.bit(7);
  }
}
