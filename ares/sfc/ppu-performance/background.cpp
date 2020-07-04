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
  uint colorShift = 3 + io.mode;
  int width = 256 << hires;

  uint tileHeight = 3 + io.tileSize;
  uint tileWidth = !hires ? tileHeight : 4;
  uint tileMask = 0x0fff >> io.mode;
  uint tiledataIndex = io.tiledataAddress >> 3 + io.mode;

  uint paletteBase = ppu.io.bgMode == 0 ? id << 5 : 0;
  uint paletteShift = 2 << io.mode;

  uint hscroll = io.hoffset;
  uint vscroll = io.voffset;
  uint hmask = (width << io.tileSize << !!(io.screenSize & 1)) - 1;
  uint vmask = (width << io.tileSize << !!(io.screenSize & 2)) - 1;

  uint y = ppu.vcounter();
  if(hires) {
    hscroll <<= 1;
    if(ppu.io.interlace) y = y << 1 | (ppu.field() && !io.mosaicEnable);
  }
  if(io.mosaicEnable) {
    y -= ppu.mosaic.voffset() << (hires && ppu.io.interlace);
  }

  uint mosaicCounter = 1;
  uint mosaicPalette = 0;
  uint mosaicPriority = 0;
  uint15 mosaicColor = 0;

  int x = 0 - (hscroll & 7);
  while(x < width) {
    uint hoffset = x + hscroll;
    uint voffset = y + vscroll;
    if(offsetPerTileMode) {
      uint validBit = 0x2000 << id;
      uint offsetX = x + (hscroll & 7);
      if(offsetX >= 8) {  //first column is exempt
        uint hlookup = ppu.bg3.getTile((offsetX - 8) + (ppu.bg3.io.hoffset & ~7), ppu.bg3.io.voffset + 0);
        if(ppu.io.bgMode == 4) {
          if(hlookup & validBit) {
            if(!(hlookup & 0x8000)) {
              hoffset = offsetX + (hlookup & ~7);
            } else {
              voffset = y + hlookup;
            }
          }
        } else {
          uint vlookup = ppu.bg3.getTile((offsetX - 8) + (ppu.bg3.io.hoffset & ~7), ppu.bg3.io.voffset + 8);
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

    uint tileNumber = getTile(hoffset, voffset);
    uint mirrorY = tileNumber & 0x8000 ? 7 : 0;
    uint mirrorX = tileNumber & 0x4000 ? 7 : 0;
    uint8 tilePriority = io.priority[bool(tileNumber & 0x2000)];
    uint paletteNumber = tileNumber >> 10 & 7;
    uint paletteIndex = paletteBase + (paletteNumber << paletteShift) & 0xff;

    if(tileWidth  == 4 && (bool(hoffset & 8) ^ bool(mirrorX))) tileNumber +=  1;
    if(tileHeight == 4 && (bool(voffset & 8) ^ bool(mirrorY))) tileNumber += 16;
    tileNumber = (tileNumber & 0x03ff) + tiledataIndex & tileMask;

    uint16 address;
    address = (tileNumber << colorShift) + (voffset & 7 ^ mirrorY);

    uint64 data;
    data |= (uint64)ppu.vram[address +  0] <<  0;
    data |= (uint64)ppu.vram[address +  8] << 16;
    data |= (uint64)ppu.vram[address + 16] << 32;
    data |= (uint64)ppu.vram[address + 24] << 48;

    for(uint tileX = 0; tileX < 8; tileX++, x++) {
      if(x & width) continue;  //x < 0 || x >= width
      if(--mosaicCounter == 0) {
        uint color = 0, shift = mirrorX ? tileX : 7 - tileX;
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
        uint X = x >> 1;
        if(x & 1) {
          if(io.aboveEnable && !windowAbove[X]) ppu.dac.plotAbove(X, id, mosaicPriority, mosaicColor);
        } else {
          if(io.belowEnable && !windowBelow[X]) ppu.dac.plotBelow(X, id, mosaicPriority, mosaicColor);
        }
      }
    }
  }
}

auto PPU::Background::getTile(uint hoffset, uint voffset) -> uint16 {
  bool hires = ppu.io.bgMode == 5 || ppu.io.bgMode == 6;
  uint tileHeight = 3 + io.tileSize;
  uint tileWidth = !hires ? tileHeight : 4;
  uint screenX = io.screenSize & 1 ? 32 << 5 : 0;
  uint screenY = io.screenSize & 2 ? 32 << 5 + (io.screenSize & 1) : 0;
  uint tileX = hoffset >> tileWidth;
  uint tileY = voffset >> tileHeight;
  uint offset = (tileY & 0x1f) << 5 | (tileX & 0x1f);
  if(tileX & 0x20) offset += screenX;
  if(tileY & 0x20) offset += screenY;
  return ppu.vram[io.screenAddress + offset];
}

auto PPU::Background::power() -> void {
  io = {};
  window = {};
}
