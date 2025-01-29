auto PPU::enable() const -> bool {
  return io.bgEnable || io.spriteEnable;
}

auto PPU::rendering() const -> bool {
  return enable() && (io.ly < 240 || io.ly == vlines() - 1);
}

auto PPU::loadCHR(n16 address) -> n8 {
  if (enable()) {
    io.busAddress = (n14)address;
    cartridge.ppuAddressBus(address);
    return cartridge.readCHR(address);
  } else {
    return 0x00;
  }
}

auto PPU::renderPixel() -> void {
  if(io.ly >= screen->canvasHeight()) return;

  u32  x = io.lx - 1;
  u32  mask = 0x8000 >> (scroll.fineX + (x & 7));
  u32  palette = 0;
  u32  objectPalette = 0;
  bool objectPriority = 0;

  //PAL systems technically blank the topmost scanline and a 2px column on each side of active display.
  //This does little else but crop good tiledata and mismatch screenshots with ntsc.
  //Uncomment the line below to enable.
  //if(Region::PAL()) if(io.ly == 0 || x < 2 || x > 253) return;

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

  u32 color = 0;
  if (enable() || (n14)var.address < 0x3f00) {
    color = io.emphasis << 6 | readCGRAM(palette);
  } else {
    color = io.emphasis << 6 | readCGRAM((n5)var.address);
  }

  if(Region::PAL())
    output[(x + 18) % 283] = color;
  else
    output[(x + 16) % 283] = color;
}

auto PPU::renderScanline() -> void {
  if(io.ly < screen->canvasHeight()) {
    output = screen->pixels().data() + io.ly * 283;
    for (auto n : range(283))
      output[n] = Region::PAL() ? 0x3f : io.emphasis << 6 | readCGRAM(0);
  }

  //Vblank
  if(io.ly >= 240 && io.ly <= vlines() - 2) return step(341), scanline();

  //  0
  step(1);

  // force clear sprite counter at start of each scanline
  for (auto& id : latch.oamId) id = 64;

  //  1-256
  for(u32 tile : range(32)) {
    u32 nametable = loadCHR(0x2000 | (n12)var.address);
    u32 tileaddr = io.bgAddress | nametable << 4 | var.fineY;
    renderPixel();
    step(1);

    renderPixel();
    step(1);

    u32 attribute = loadCHR(0x23c0 | var.nametable << 10 | var.attrY << 3 | var.attrX);
    if(var.tileY & 2) attribute >>= 4;
    if(var.tileX & 2) attribute >>= 2;
    renderPixel();
    step(1);

    renderPixel();
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
    step(1);

    latch.nametable = latch.nametable << 8 | nametable;
    latch.attribute = latch.attribute << 2 | (attribute & 3);
    latch.tiledataLo = latch.tiledataLo << 8 | tiledataLo;
    latch.tiledataHi = latch.tiledataHi << 8 | tiledataHi;
  }

  for(u32 n : range(8)) {
    latch.oam[n].id   = latch.oamId[n];
    latch.oam[n].y    = soam[4 * n + 0];
    latch.oam[n].tile = soam[4 * n + 1];
    latch.oam[n].attr = soam[4 * n + 2];
    latch.oam[n].x    = soam[4 * n + 3];
  }

  //257-320
  for(u32 sprite : range(8)) {
    u32 nametable = loadCHR(0x2000 | (n12)var.address);
    step(2);

    u32 attribute = loadCHR(0x23c0 | var.nametable << 10 | (var.tileY >> 2) << 3 | var.tileX >> 2);
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
  }

  //321-336
  for(u32 tile : range(2)) {
    u32 nametable = loadCHR(0x2000 | (n12)var.address);
    u32 tileaddr = io.bgAddress | nametable << 4 | var.fineY;
    step(2);

    u32 attribute = loadCHR(0x23c0 | var.nametable << 10 | (var.tileY >> 2) << 3 | var.tileX >> 2);
    if(var.tileY & 2) attribute >>= 4;
    if(var.tileX & 2) attribute >>= 2;
    step(2);

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
  loadCHR(0x2000 | (n12)var.address);
  bool skip = !Region::PAL() && enable() && io.field == 1 && io.ly == vlines() - 1;
  step(2);

  //339
  loadCHR(0x2000 | (n12)var.address);
  step(1);

  //340
  if(!skip) step(1);

  return scanline();
}
