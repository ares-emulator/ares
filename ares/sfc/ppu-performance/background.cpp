auto PPU::Background::render() -> void {
  if(!io.aboveEnable && !io.belowEnable) return;
  if(io.mode == Mode::Inactive) return;
  if(io.mode == Mode::Mode7) return renderMode7();

  bool windowAbove[256];
  bool windowBelow[256];
  ppu.window.render(window, window.aboveEnable, windowAbove);
  ppu.window.render(window, window.belowEnable, windowBelow);

  bool hires = ppu.io.bgMode == 5 || ppu.io.bgMode == 6;
  bool offsetPerTileMode = ppu.io.bgMode == 2 || ppu.io.bgMode == 4 || ppu.io.bgMode == 6;
  bool directColorMode = ppu.dac.io.directColor && id == ID::BG1 && (ppu.io.bgMode == 3 || ppu.io.bgMode == 4);
  u32 colorShift = 3 + io.mode;
  s32 width = 256 << hires;

  u32 tileHeight = 3 + io.tileSize;
  u32 tileWidth = !hires ? tileHeight : 4;
  u32 tileMask = 0x0fff >> io.mode;
  u32 tiledataIndex = io.tiledataAddress >> 3 + io.mode;

  u32 paletteBase = ppu.io.bgMode == 0 ? id << 5 : 0;
  u32 paletteShift = 2 << io.mode;

  u32 hscroll = io.hoffset;
  u32 vscroll = io.voffset;
  u32 hmask = (width << io.tileSize << !!(io.screenSize & 1)) - 1;
  u32 vmask = (width << io.tileSize << !!(io.screenSize & 2)) - 1;

  u32 y = ppu.vcounter();
  if(hires) {
    hscroll <<= 1;
    if(ppu.io.interlace) y = y << 1 | (ppu.field() && !io.mosaicEnable);
  }
  if(io.mosaicEnable) {
    y -= ppu.mosaic.voffset() << (hires && ppu.io.interlace);
  }

  u32 mosaicCounter = 1;
  u32 mosaicPalette = 0;
  u32 mosaicPriority = 0;
  n15 mosaicColor = 0;

  s32 x = 0 - (hscroll & 7);
  while(x < width) {
    u32 hoffset = x + hscroll;
    u32 voffset = y + vscroll;
    if(offsetPerTileMode) {
      u32 validBit = 0x2000 << id;
      u32 offsetX = x + (hscroll & 7);
      if(offsetX >= 8) {  //first column is exempt
        u32 hlookup = ppu.bg3.getTile((offsetX - 8) + (ppu.bg3.io.hoffset & ~7), ppu.bg3.io.voffset + 0);
        if(ppu.io.bgMode == 4) {
          if(hlookup & validBit) {
            if(!(hlookup & 0x8000)) {
              hoffset = offsetX + (hlookup & ~7);
            } else {
              voffset = y + hlookup;
            }
          }
        } else {
          u32 vlookup = ppu.bg3.getTile((offsetX - 8) + (ppu.bg3.io.hoffset & ~7), ppu.bg3.io.voffset + 8);
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
    n8  tilePriority = io.priority[bool(tileNumber & 0x2000)];
    u32 paletteNumber = tileNumber >> 10 & 7;
    u32 paletteIndex = paletteBase + (paletteNumber << paletteShift) & 0xff;

    if(tileWidth  == 4 && (bool(hoffset & 8) ^ bool(mirrorX))) tileNumber +=  1;
    if(tileHeight == 4 && (bool(voffset & 8) ^ bool(mirrorY))) tileNumber += 16;
    tileNumber = (tileNumber & 0x03ff) + tiledataIndex & tileMask;

    n16 address;
    address = (tileNumber << colorShift) + (voffset & 7 ^ mirrorY);

    n64 data;
    data |= (n64)ppu.vram[address +  0] <<  0;
    data |= (n64)ppu.vram[address +  8] << 16;
    data |= (n64)ppu.vram[address + 16] << 32;
    data |= (n64)ppu.vram[address + 24] << 48;

    for(u32 tileX = 0; tileX < 8; tileX++, x++) {
      if(x & width) continue;  //x < 0 || x >= width
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

        mosaicCounter = io.mosaicEnable ? ppu.mosaic.size << hires : 1;
        mosaicPalette = color;
        mosaicPriority = tilePriority;
        if(directColorMode) {
          mosaicColor = ppu.dac.directColor(paletteNumber, mosaicPalette);
        } else {
          mosaicColor = ppu.dac.cgram[paletteIndex + mosaicPalette];
        }
      }
      if(!mosaicPalette) continue;

      if(!hires) {
        if(io.aboveEnable && !windowAbove[x]) ppu.dac.plotAbove(x, id, mosaicPriority, mosaicColor);
        if(io.belowEnable && !windowBelow[x]) ppu.dac.plotBelow(x, id, mosaicPriority, mosaicColor);
      } else {
        u32 X = x >> 1;
        if(x & 1) {
          if(io.aboveEnable && !windowAbove[X]) ppu.dac.plotAbove(X, id, mosaicPriority, mosaicColor);
        } else {
          if(io.belowEnable && !windowBelow[X]) ppu.dac.plotBelow(X, id, mosaicPriority, mosaicColor);
        }
      }
    }
  }
}

auto PPU::Background::getTile(u32 hoffset, u32 voffset) -> n16 {
  bool hires = ppu.io.bgMode == 5 || ppu.io.bgMode == 6;
  u32  tileHeight = 3 + io.tileSize;
  u32  tileWidth = !hires ? tileHeight : 4;
  u32  screenX = io.screenSize & 1 ? 32 << 5 : 0;
  u32  screenY = io.screenSize & 2 ? 32 << 5 + (io.screenSize & 1) : 0;
  u32  tileX = hoffset >> tileWidth;
  u32  tileY = voffset >> tileHeight;
  u32  offset = (tileY & 0x1f) << 5 | (tileX & 0x1f);
  if(tileX & 0x20) offset += screenX;
  if(tileY & 0x20) offset += screenY;
  return ppu.vram[io.screenAddress + offset];
}

auto PPU::Background::power() -> void {
  io = {};
  window = {};
}
