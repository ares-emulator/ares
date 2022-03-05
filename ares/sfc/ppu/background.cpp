inline auto PPU::Background::hires() const -> bool {
  return self.io.bgMode == 5 || self.io.bgMode == 6;
}

//V = 0, H = 0
auto PPU::Background::frame() -> void {
}

//H = 0
auto PPU::Background::scanline() -> void {
  mosaic.hcounter = self.mosaic.size;
  mosaic.hoffset = 0;

  renderingIndex = 0;
  pixelCounter = (io.hoffset & 7) << hires();

  opt.hoffset = 0;
  opt.voffset = 0;
}

//H = 56
auto PPU::Background::begin() -> void {
  //remove partial tile columns that have been scrolled offscreen
  for(auto& data : tiles[0].data) data >>= pixelCounter << 1;
}

auto PPU::Background::fetchNameTable() -> void {
  if(self.vcounter() == 0) return;

  u32 nameTableIndex = self.hcounter() >> 5 << hires();
  s32 x = (self.hcounter() & ~31) >> 2;

  u32 hpixel = x << hires();
  u32 vpixel = self.vcounter();
  u32 hscroll = io.hoffset;
  u32 vscroll = io.voffset;

  if(hires()) {
    hscroll <<= 1;
    if(self.io.interlace) vpixel = vpixel << 1 | (self.field() && !mosaic.enable);
  }
  if(mosaic.enable) {
    vpixel -= self.mosaic.voffset() << (hires() && self.io.interlace);
  }

  bool repeated = false;
  repeat:

  u32 hoffset = hpixel + hscroll;
  u32 voffset = vpixel + vscroll;

  if(self.io.bgMode == 2 || self.io.bgMode == 4 || self.io.bgMode == 6) {
    auto hlookup = self.bg3.opt.hoffset;
    auto vlookup = self.bg3.opt.voffset;
    u32  valid = 13 + id;

    if(self.io.bgMode == 4) {
      if(hlookup.bit(valid)) {
        if(!hlookup.bit(15)) {
          hoffset = hpixel + (hlookup & ~7) + (hscroll & 7);
        } else {
          voffset = vpixel + (vlookup);
        }
      }
    } else {
      if(hlookup.bit(valid)) hoffset = hpixel + (hlookup & ~7) + (hscroll & 7);
      if(vlookup.bit(valid)) voffset = vpixel + (vlookup);
    }
  }

  u32 width = 256 << hires();
  u32 hsize = width << io.tileSize << io.screenSize.bit(0);
  u32 vsize = width << io.tileSize << io.screenSize.bit(1);

  hoffset &= hsize - 1;
  voffset &= vsize - 1;

  u32 vtiles = 3 + io.tileSize;
  u32 htiles = !hires() ? vtiles : 4;

  u32 htile = hoffset >> htiles;
  u32 vtile = voffset >> vtiles;

  u32 hscreen = io.screenSize.bit(0) ? 32 << 5 : 0;
  u32 vscreen = io.screenSize.bit(1) ? 32 << 5 + io.screenSize.bit(0) : 0;

  n16 offset = (n5)htile << 0 | (n5)vtile << 5;
  if(htile & 0x20) offset += hscreen;
  if(vtile & 0x20) offset += vscreen;

  n16 address = io.screenAddress + offset;
  n16 attributes = vram[address];

  auto& tile = tiles[nameTableIndex];
  tile.character = attributes.bit(0,9);
  tile.paletteGroup = attributes.bit(10,12);
  tile.priority = io.priority[attributes.bit(13)];
  tile.hmirror = attributes.bit(14);
  tile.vmirror = attributes.bit(15);

  if(htiles == 4 && bool(hoffset & 8) != tile.hmirror) tile.character +=  1;
  if(vtiles == 4 && bool(voffset & 8) != tile.vmirror) tile.character += 16;

  u32 characterMask = vram.mask >> 3 + io.mode;
  u32 characterIndex = io.tiledataAddress >> 3 + io.mode;
  n16 origin = tile.character + characterIndex & characterMask;

  if(tile.vmirror) voffset ^= 7;
  tile.address = (origin << 3 + io.mode) + (voffset & 7);

  u32 paletteOffset = self.io.bgMode == 0 ? id << 5 : 0;
  u32 paletteSize = 2 << io.mode;
  tile.palette = paletteOffset + (tile.paletteGroup << paletteSize);

  nameTableIndex++;
  if(hires() && !repeated) {
    repeated = true;
    hpixel += 8;
    goto repeat;
  }
}

auto PPU::Background::fetchOffset(u32 y) -> void {
  if(self.vcounter() == 0) return;

  u32 characterIndex = self.hcounter() >> 5 << hires();
  u32 x = characterIndex << 3;

  u32 hoffset = x + (io.hoffset & ~7);
  u32 voffset = y + (io.voffset);

  u32 vtiles = 3 + io.tileSize;
  u32 htiles = !hires() ? vtiles : 4;

  u32 htile = hoffset >> htiles;
  u32 vtile = voffset >> vtiles;

  u32 hscreen = io.screenSize.bit(0) ? 32 << 5 : 0;
  u32 vscreen = io.screenSize.bit(1) ? 32 << 5 + io.screenSize.bit(0) : 0;

  n16 offset = (n5)htile << 0 | (n5)vtile << 5;
  if(htile & 0x20) offset += hscreen;
  if(vtile & 0x20) offset += vscreen;

  n16 address = io.screenAddress + offset;
  if(y == 0) opt.hoffset = vram[address];
  if(y == 8) opt.voffset = vram[address];
}

auto PPU::Background::fetchCharacter(u32 index, bool half) -> void {
  if(self.vcounter() == 0) return;

  u32 characterIndex = (self.hcounter() >> 5 << hires()) + half;

  auto& tile = tiles[characterIndex];
  n16 data = vram[tile.address + (index << 3)];

  //reverse bits so that the lowest bit is the left-most pixel
  if(!tile.hmirror) {
    data = data >> 4 & 0x0f0f | data << 4 & 0xf0f0;
    data = data >> 2 & 0x3333 | data << 2 & 0xcccc;
    data = data >> 1 & 0x5555 | data << 1 & 0xaaaa;
  }

  //interleave two bitplanes for faster planar decoding later
  tile.data[index] = (
    ((n8(data >> 0) * 0x0101010101010101ull & 0x8040201008040201ull) * 0x0102040810204081ull >> 49) & 0x5555
  | ((n8(data >> 8) * 0x0101010101010101ull & 0x8040201008040201ull) * 0x0102040810204081ull >> 48) & 0xaaaa
  );
}

auto PPU::Background::run(bool screen) -> void {
  if(self.vcounter() == 0) return;

  if(screen == Screen::Below) {
    output.above.priority = 0;
    output.below.priority = 0;
    if(!hires()) return;
  }

  if(io.mode == Mode::Mode7) return runMode7();

  auto& tile = tiles[renderingIndex];
  n8 color;
  if(io.mode >= Mode::BPP2) color.bit(0,1) = tile.data[0] & 3; tile.data[0] >>= 2;
  if(io.mode >= Mode::BPP4) color.bit(2,3) = tile.data[1] & 3; tile.data[1] >>= 2;
  if(io.mode >= Mode::BPP8) color.bit(4,5) = tile.data[2] & 3; tile.data[2] >>= 2;
  if(io.mode >= Mode::BPP8) color.bit(6,7) = tile.data[3] & 3; tile.data[3] >>= 2;

  Pixel pixel;
  pixel.priority = tile.priority;
  pixel.palette = color ? u32(tile.palette + color) : 0;
  pixel.paletteGroup = tile.paletteGroup;
  if(++pixelCounter == 0) renderingIndex++;

  u32 x = self.hcounter() - 56 >> 2;
  if(x == 0 && (!hires() || screen == Screen::Below)) {
    mosaic.hcounter = self.mosaic.size;
    mosaic.pixel = pixel;
  } else if((!hires() || screen == Screen::Below) && --mosaic.hcounter == 0) {
    mosaic.hcounter = self.mosaic.size;
    mosaic.pixel = pixel;
  } else if(mosaic.enable) {
    pixel = mosaic.pixel;
  }
  if(screen == Screen::Above) x++;
  if(pixel.palette == 0) return;

  if(!hires() || screen == Screen::Above) if(io.aboveEnable) output.above = pixel;
  if(!hires() || screen == Screen::Below) if(io.belowEnable) output.below = pixel;
}

auto PPU::Background::power() -> void {
  io = {};
  io.tiledataAddress = (random() & 0x0f) << 12;
  io.screenAddress = (random() & 0xfc) << 8;
  io.screenSize = random();
  io.tileSize = random();
  io.aboveEnable = random();
  io.belowEnable = random();
  io.hoffset = random();
  io.voffset = random();

  output.above = {};
  output.below = {};

  mosaic = {};
  mosaic.enable = random();
}
