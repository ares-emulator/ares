auto PPU::Background::render() -> void {
  if(!io.aboveEnable && !io.belowEnable) return;
  if(io.mode == Mode::Inactive) return;
  if(io.mode == Mode::Mode7) return renderMode7();

  bool windowAbove[448];
  bool windowBelow[448];
  self.window.render(window, window.aboveEnable, windowAbove);
  self.window.render(window, window.belowEnable, windowBelow);

  bool hires = self.io.bgMode == 5 || self.io.bgMode == 6;
  bool offsetPerTileMode = self.io.bgMode == 2 || self.io.bgMode == 4 || self.io.bgMode == 6;
  bool directColorMode = self.dac.io.directColor && id == ID::BG1 && (self.io.bgMode == 3 || self.io.bgMode == 4);
  u32 colorShift = 3 + io.mode;
  s32 width = self.width() << hires;

  u32 tileHeight = 3 + io.tileSize;
  u32 tileWidth = !hires ? tileHeight : 4;
  u32 tileMask = 0x0fff >> io.mode;
  u32 tiledataIndex = io.tiledataAddress >> 3 + io.mode;

  u32 paletteBase = self.io.bgMode == 0 ? id << 5 : 0;
  u32 paletteShift = 2 << io.mode;

  u32 hscroll = io.hoffset;
  u32 vscroll = io.voffset;
  u32 hmask = (256 << hires << io.tileSize << !!(io.screenSize & 1)) - 1;
  u32 vmask = (256 << hires << io.tileSize << !!(io.screenSize & 2)) - 1;

  u32 y = self.vcounter();
  if(hires) {
    hscroll <<= 1;
    if(self.io.interlace) y = y << 1 | (self.field() && !io.mosaicEnable);
  }
  if(io.mosaicEnable) {
    y -= self.mosaic.voffset() << (hires && self.io.interlace);
  }

  u32 mosaicCounter = 1;
  u32 mosaicPalette = 0;
  u32 mosaicPriority = 0;
  n15 mosaicColor = 0;

  s32 x1 =   0;
  s32 x2 = 256;
  if(self.width() == 352) x1 = -48, x2 = 304;
  if(self.width() == 448) x1 = -96, x2 = 352;
  x1 <<= hires;
  x2 <<= hires;

  s32 x = x1 - (hscroll & 7);
  while(x < x2) {
    u32 hoffset = x + hscroll;
    u32 voffset = y + vscroll;
    if(offsetPerTileMode) {
      u32 validBit = 0x2000 << id;
      u32 offsetX = x + (hscroll & 7);
      if(offsetX >= 8) {  //first column is exempt
        u32 hlookup = self.bg3.getTile((offsetX - 8) + (self.bg3.io.hoffset & ~7), self.bg3.io.voffset + 0);
        if(self.io.bgMode == 4) {
          if(hlookup & validBit) {
            if(!(hlookup & 0x8000)) {
              hoffset = offsetX + (hlookup & ~7);
            } else {
              voffset = y + hlookup;
            }
          }
        } else {
          u32 vlookup = self.bg3.getTile((offsetX - 8) + (self.bg3.io.hoffset & ~7), self.bg3.io.voffset + 8);
          if(hlookup & validBit) {
            hoffset = offsetX + (hlookup & ~7);
          }
          if(vlookup & validBit) {
            voffset = y + vlookup;
          }
        }
      }
    }
    hoffset &= hmask;
    voffset &= vmask;

    u32 tileNumber = getTile(hoffset, voffset);
    u32 mirrorY = tileNumber & 0x8000 ? 7 : 0;
    u32 mirrorX = tileNumber & 0x4000 ? 7 : 0;
    u8  tilePriority = io.priority[bool(tileNumber & 0x2000)];
    u32 paletteNumber = tileNumber >> 10 & 7;
    u32 paletteIndex = paletteBase + (paletteNumber << paletteShift) & 0xff;

    if(tileWidth  == 4 && (bool(hoffset & 8) ^ bool(mirrorX))) tileNumber +=  1;
    if(tileHeight == 4 && (bool(voffset & 8) ^ bool(mirrorY))) tileNumber += 16;
    tileNumber = (tileNumber & 0x03ff) + tiledataIndex & tileMask;

    n16 address;
    address = (tileNumber << colorShift) + (voffset & 7 ^ mirrorY);

    n64 data;
    data |= (n64)self.vram[address +  0] <<  0;
    data |= (n64)self.vram[address +  8] << 16;
    data |= (n64)self.vram[address + 16] << 32;
    data |= (n64)self.vram[address + 24] << 48;

    for(u32 tileX = 0; tileX < 8; tileX++, x++) {
      if(x < x1 || x >= x2) continue;
      if(--mosaicCounter == 0) {
        u32 color = 0, shift = mirrorX ? tileX : 7 - tileX;
      /*if(io.mode >= Mode::BPP2)*/ {
          color += data >> shift +  0 &   1;
          color += data >> shift +  7 &   2;
        }
        if(io.mode >= Mode::BPP4) {
          color += data >> shift + 14 &   4;
          color += data >> shift + 21 &   8;
        }
        if(io.mode >= Mode::BPP8) {
          color += data >> shift + 28 &  16;
          color += data >> shift + 35 &  32;
          color += data >> shift + 42 &  64;
          color += data >> shift + 49 & 128;
        }

        mosaicCounter = io.mosaicEnable ? self.mosaic.size << hires : 1;
        mosaicPalette = color;
        mosaicPriority = tilePriority;
        if(directColorMode) {
          mosaicColor = self.dac.directColor(mosaicPalette, paletteNumber);
        } else {
          mosaicColor = self.dac.cgram[paletteIndex + mosaicPalette];
        }
      }
      if(!mosaicPalette) continue;

      s32 xp = x + abs(x1);
      if(!hires) {
        if(io.aboveEnable && !windowAbove[xp]) self.dac.plotAbove(xp, id, mosaicPriority, mosaicColor);
        if(io.belowEnable && !windowBelow[xp]) self.dac.plotBelow(xp, id, mosaicPriority, mosaicColor);
      } else {
        u32 Xp = xp >> 1;
        if(xp & 1) {
          if(io.aboveEnable && !windowAbove[Xp]) self.dac.plotAbove(Xp, id, mosaicPriority, mosaicColor);
        } else {
          if(io.belowEnable && !windowBelow[Xp]) self.dac.plotBelow(Xp, id, mosaicPriority, mosaicColor);
        }
      }
    }
  }
}

auto PPU::Background::getTile(u32 hoffset, u32 voffset) -> n16 {
  bool hires = self.io.bgMode == 5 || self.io.bgMode == 6;
  u32  tileHeight = 3 + io.tileSize;
  u32  tileWidth = !hires ? tileHeight : 4;
  u32  screenX = io.screenSize & 1 ? 32 << 5 : 0;
  u32  screenY = io.screenSize & 2 ? 32 << 5 + (io.screenSize & 1) : 0;
  u32  tileX = hoffset >> tileWidth;
  u32  tileY = voffset >> tileHeight;
  u32  offset = (tileY & 0x1f) << 5 | (tileX & 0x1f);
  if(tileX & 0x20) offset += screenX;
  if(tileY & 0x20) offset += screenY;
  return self.vram[io.screenAddress + offset];
}

auto PPU::Background::power() -> void {
  io = {};
  window = {};
}
