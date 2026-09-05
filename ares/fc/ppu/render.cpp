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

auto PPU::bgShift() -> u32 {
  if(!enable()) return 0;

  u32 mask = 0x8000 >> scroll.fineX;
  u32 palette = 0;

  palette |= latch.tiledataLo  & mask ? 1 : 0;
  palette |= latch.tiledataHi  & mask ? 2 : 0;
  palette |= latch.attributeLo & mask ? 4 : 0;
  palette |= latch.attributeHi & mask ? 8 : 0;
  latch.tiledataLo <<= 1;
  latch.tiledataHi <<= 1;
  latch.tiledataHi |= 0x0001;
  latch.attributeLo <<= 1;
  latch.attributeHi <<= 1;

  return palette;
}

auto PPU::renderPixel() -> void {
  if(io.ly >= screen->canvasHeight()) return;

  u32  x = io.lx - 1;
  u32  objectPalette = 0;
  bool objectPriority = 0;

  u32 palette = bgShift();
  if(!(palette & 3)) palette = 0;
  if(!io.bgEnable) palette = 0;
  if(!io.bgEdgeEnable && x < 8) palette = 0;

  if(!model->raster.pixelVisible(x, io.ly)) return;

  for(i32 sprite = 7; sprite >= 0; sprite--) {
    if(latch.oam[sprite].id == 64) continue;
    if(!latch.oam[sprite].x) latch.oam[sprite].counting = false;
    if(latch.oam[sprite].counting) {
      latch.oam[sprite].x--;
      continue;
    }
    if(!enable()) continue;

    //shift out sprite pixels once x-position countdown expires
    u32 spritePalette = 0;
    spritePalette |= latch.oam[sprite].tiledataLo & 0x80 ? 1 : 0;
    spritePalette |= latch.oam[sprite].tiledataHi & 0x80 ? 2 : 0;
    latch.oam[sprite].tiledataLo <<= 1;
    latch.oam[sprite].tiledataHi <<= 1;

    if(!io.spriteEnable) continue;
    if(!io.spriteEdgeEnable && x < 8) continue;
    if(spritePalette == 0) continue;

    if(latch.oam[sprite].id == 0 && palette && x != 255) io.spriteZeroHit = 1;
    spritePalette |= (latch.oam[sprite].attr & 3) << 2;
    objectPriority = latch.oam[sprite].attr & 0x20;
    objectPalette = 16 + spritePalette;
  }

  if(objectPalette && (palette == 0 || objectPriority == 0)) palette = objectPalette;

  u32 color = 0;
  if(enable() || (n14)var.address < 0x3f00) {
    color = io.emphasis << 6 | readCGRAM(palette);
  } else {
    color = io.emphasis << 6 | readCGRAM((n5)var.address);
  }

  output[(x + model->raster.outputOffset) % 283] = color;
}

auto PPU::renderScanline() -> void {
  if(io.ly < screen->canvasHeight()) {
    output = screen->pixels().data() + io.ly * 283;
    auto backdrop = model->raster.backdrop(io.emphasis << 6 | readCGRAM(0));
    for(auto n : range(283)) output[n] = backdrop;
  }

  //Vblank
  if(io.ly >= 240 && io.ly <= vlines() - 2) return step(341), scanline();

  //  0
  step(1);

  //force clear sprite counter at start of each scanline
  for(auto& id : latch.oamId) id = 64;

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

    latch.attributeLo.byte(0) = (attribute & 1) ? 0xff : 0x00;
    latch.attributeHi.byte(0) = (attribute & 2) ? 0xff : 0x00;
    latch.tiledataLo.byte(0) = tiledataLo;
    latch.tiledataHi.byte(0) = tiledataHi;
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
    if(latch.oam[sprite].attr & 0x40) latch.oam[sprite].tiledataLo = bit::reverse<u8>(latch.oam[sprite].tiledataLo);
    step(2);

    latch.oam[sprite].tiledataHi = loadCHR(tileaddr + 8);
    if(latch.oam[sprite].attr & 0x40) latch.oam[sprite].tiledataHi = bit::reverse<u8>(latch.oam[sprite].tiledataHi);
    step(2);
  }

  //321-336
  for(u32 tile : range(2)) {
    u32 nametable = loadCHR(0x2000 | (n12)var.address);
    u32 tileaddr = io.bgAddress | nametable << 4 | var.fineY;
    bgShift();
    step(1);
    bgShift();
    step(1);

    u32 attribute = loadCHR(0x23c0 | var.nametable << 10 | (var.tileY >> 2) << 3 | var.tileX >> 2);
    if(var.tileY & 2) attribute >>= 4;
    if(var.tileX & 2) attribute >>= 2;
    bgShift();
    step(1);
    bgShift();
    step(1);

    u32 tiledataLo = loadCHR(tileaddr + 0);
    bgShift();
    step(1);
    bgShift();
    step(1);

    u32 tiledataHi = loadCHR(tileaddr + 8);
    bgShift();
    step(1);
    bgShift();
    step(1);

    latch.attributeLo.byte(0) = (attribute & 1) ? 0xff : 0x00;
    latch.attributeHi.byte(0) = (attribute & 2) ? 0xff : 0x00;
    latch.tiledataLo.byte(0) = tiledataLo;
    latch.tiledataHi.byte(0) = tiledataHi;
  }

  //337-338
  loadCHR(0x2000 | (n12)var.address);
  bool skip = model->raster.oddFrameCycleSkip && enable() && io.field == 1 && io.ly == vlines() - 1;
  step(2);

  //339
  loadCHR(0x2000 | (n12)var.address);
  if(enable()) {
    for(u32 sprite : range(8)) latch.oam[sprite].counting = true;
  }
  step(1);

  //340
  if(!skip) step(1);

  return scanline();
}
