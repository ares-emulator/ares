auto PPU::enable() const -> bool {
  return io.bgEnable || io.spriteEnable;
}

auto PPU::loadCHR(n16 address) -> n8 {
  return enable() ? cartridge.readCHR(address) : (n8)0x00;
}

auto PPU::renderPixel() -> void {
  //todo: renderPixel() is called when Y=261 ... why?
  if(io.ly >= screen->canvasHeight()) return;
  auto output = screen->pixels().data() + io.ly * 256;

  u32  x = io.lx - 1;
  u32  mask = 0x8000 >> (io.v.fineX + (x & 7));
  u32  palette = 0;
  u32  objectPalette = 0;
  bool objectPriority = 0;

  palette |= latch.tiledataLo & mask ? 1 : 0;
  palette |= latch.tiledataHi & mask ? 2 : 0;
  if(palette) {
    u32 attr = latch.attribute;
    if(mask >= 256) attr >>= 2;
    palette |= (attr & 3) << 2;
  }

  if(!io.bgEnable) palette = 0;
  if(!io.bgEdgeEnable && x < 8) palette = 0;

  if(io.spriteEnable)
  for(i32 sprite = 7; sprite >= 0; sprite--) {
    if(!io.spriteEdgeEnable && x < 8) continue;
    if(latch.oam[sprite].id == 64) continue;

    u32 spriteX = x - latch.oam[sprite].x;
    if(spriteX >= 8) continue;

    if(latch.oam[sprite].attr & 0x40) spriteX ^= 7;
    u32 mask = 0x80 >> spriteX;
    u32 spritePalette = 0;
    spritePalette |= latch.oam[sprite].tiledataLo & mask ? 1 : 0;
    spritePalette |= latch.oam[sprite].tiledataHi & mask ? 2 : 0;
    if(spritePalette == 0) continue;

    if(latch.oam[sprite].id == 0 && palette && x != 255) io.spriteZeroHit = 1;
    spritePalette |= (latch.oam[sprite].attr & 3) << 2;

    objectPriority = latch.oam[sprite].attr & 0x20;
    objectPalette = 16 + spritePalette;
  }

  if(objectPalette) {
    if(palette == 0 || objectPriority == 0) palette = objectPalette;
  }

  if(!enable()) palette = 0;
  output[x] = io.emphasis << 6 | readCGRAM(palette);
}

auto PPU::renderSprite() -> void {
  if(!enable()) return;

  u32 n = latch.oamIterator++;
  s32 ly = io.ly == vlines() - 1 ? -1 : (s32)io.ly;
  u32 y = ly - oam[n * 4 + 0];

  if(y >= io.spriteHeight) return;
  if(latch.oamCounter == 8) {
    io.spriteOverflow = 1;
    return;
  }

  auto& o = latch.soam[latch.oamCounter++];
  o.id   = n;
  o.y    = oam[n * 4 + 0];
  o.tile = oam[n * 4 + 1];
  o.attr = oam[n * 4 + 2];
  o.x    = oam[n * 4 + 3];
}

auto PPU::renderScanline() -> void {
  //Vblank
  if(io.ly >= 240 && io.ly <= vlines() - 2) return step(341), scanline();

  latch.oamIterator = 0;
  latch.oamCounter = 0;

  for(auto n : range(8)) latch.soam[n] = {};

  //  0
  step(1);

  //  1-256
  for(u32 tile : range(32)) {
    u32 nametable = loadCHR(0x2000 | (n12)io.v.address);
    u32 tileaddr = io.bgAddress | nametable << 4 | io.v.fineY;
    renderPixel();
    step(1);

    renderPixel();
    step(1);

    u32 attribute = loadCHR(0x23c0 | io.v.nametable << 10 | (io.v.tileY >> 2) << 3 | io.v.tileX >> 2);
    if(io.v.tileY & 2) attribute >>= 4;
    if(io.v.tileX & 2) attribute >>= 2;
    renderPixel();
    step(1);

    if(enable() && ++io.v.tileX == 0) io.v.nametableX++;
    if(enable() && tile == 31 && ++io.v.fineY == 0 && ++io.v.tileY == 30) io.v.nametableY++, io.v.tileY = 0;
    renderPixel();
    renderSprite();
    step(1);

    u32 tiledataLo = loadCHR(tileaddr + 0);
    renderPixel();
    step(1);

    renderPixel();
    step(1);

    u32 tiledataHi = loadCHR(tileaddr + 8);
    renderPixel();
    step(1);

    renderPixel();
    renderSprite();
    step(1);

    latch.nametable = latch.nametable << 8 | nametable;
    latch.attribute = latch.attribute << 2 | (attribute & 3);
    latch.tiledataLo = latch.tiledataLo << 8 | tiledataLo;
    latch.tiledataHi = latch.tiledataHi << 8 | tiledataHi;
  }

  for(u32 n : range(8)) latch.oam[n] = latch.soam[n];

  //257-320
  for(u32 sprite : range(8)) {
    u32 nametable = loadCHR(0x2000 | (n12)io.v.address);
    step(1);

    if(enable() && sprite == 0) {
      //258
      io.v.nametableX = io.t.nametableX;
      io.v.tileX = io.t.tileX;
    }
    step(1);

    u32 attribute = loadCHR(0x23c0 | io.v.nametable << 10 | (io.v.tileY >> 2) << 3 | io.v.tileX >> 2);
    u32 tileaddr = io.spriteHeight == 8
    ? io.spriteAddress + latch.oam[sprite].tile * 16
    : (latch.oam[sprite].tile & ~1) * 16 + (latch.oam[sprite].tile & 1) * 0x1000;
    step(2);

    u32 spriteY = (io.ly - latch.oam[sprite].y) & (io.spriteHeight - 1);
    if(latch.oam[sprite].attr & 0x80) spriteY ^= io.spriteHeight - 1;
    tileaddr += spriteY + (spriteY & 8);

    latch.oam[sprite].tiledataLo = loadCHR(tileaddr + 0);
    step(2);

    latch.oam[sprite].tiledataHi = loadCHR(tileaddr + 8);
    step(2);

    if(enable() && sprite == 6 && io.ly == vlines() - 1) {
      //305
      io.v.address = io.t.address;
    }
  }

  //321-336
  for(u32 tile : range(2)) {
    u32 nametable = loadCHR(0x2000 | (n12)io.v.address);
    u32 tileaddr = io.bgAddress | nametable << 4 | io.v.fineY;
    step(2);

    u32 attribute = loadCHR(0x23c0 | io.v.nametable << 10 | (io.v.tileY >> 2) << 3 | io.v.tileX >> 2);
    if(io.v.tileY & 2) attribute >>= 4;
    if(io.v.tileX & 2) attribute >>= 2;
    step(1);

    if(enable() && ++io.v.tileX == 0) io.v.nametableX++;
    step(1);

    u32 tiledataLo = loadCHR(tileaddr + 0);
    step(2);

    u32 tiledataHi = loadCHR(tileaddr + 8);
    step(2);

    latch.nametable = latch.nametable << 8 | nametable;
    latch.attribute = latch.attribute << 2 | (attribute & 3);
    latch.tiledataLo = latch.tiledataLo << 8 | tiledataLo;
    latch.tiledataHi = latch.tiledataHi << 8 | tiledataHi;
  }

  //337-338
  loadCHR(0x2000 | (n12)io.v.address);
  bool skip = !Region::PAL() && enable() && io.field == 1 && io.ly == vlines() - 1;
  step(2);

  //339
  loadCHR(0x2000 | (n12)io.v.address);
  step(1);

  //340
  if(!skip) step(1);

  return scanline();
}
