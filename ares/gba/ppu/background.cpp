//I/O settings shared by all background layers
n3 PPU::Background::IO::mode;
n1 PPU::Background::IO::frame;
n5 PPU::Background::IO::mosaicWidth;
n5 PPU::Background::IO::mosaicHeight;

auto PPU::Background::setEnable(n1 status) -> void {
  io.enable[3] = status;
  for(auto& flag : io.enable) flag &= status;
}

auto PPU::Background::scanline(u32 y) -> void {
  mosaicOffset = 0;
  for(auto& pixel : output) pixel = {};
}

auto PPU::Background::outputPixel(u32 x, u32 y) -> void {
  //horizontal mosaic
  if(!io.mosaic || !mosaicOffset) {
    mosaicOffset = 1 + io.mosaicWidth;
    mosaic = output[x];
  }
  mosaicOffset--;
}

auto PPU::Background::run(u32 x, u32 y) -> void {
  switch(id) {
  case PPU::BG0:
    if(io.mode <= 1) { linear(x, y); break; }
    break;

  case PPU::BG1:
    if(io.mode <= 1) { linear(x, y); break; }
    break;

  case PPU::BG2:
    if(io.mode == 0) { linear(x, y); break; }
    if(io.mode <= 2) { affineFetchTileMap(x, y); affineFetchTileData(x, y); break; }
    if(io.mode <= 5) { bitmap(x, y); break; }
    break;

  case PPU::BG3:
    if(io.mode == 0) { linear(x, y); break; }
    if(io.mode == 2) { affineFetchTileMap(x, y); affineFetchTileData(x, y); break; }
    break;
  }
}

auto PPU::Background::linearFetchTileMap() -> void {
  n6 tx = fx >> 3;
  n6 ty = fy >> 3;

  u32 offset = (ty & 31) << 5 | (tx & 31);
  if(io.screenSize.bit(0) && (tx & 32)) offset += 32 << 5;
  if(io.screenSize.bit(1) && (ty & 32)) offset += 32 << 5 + io.screenSize.bit(0);
  offset = (io.screenBase << 11) + (offset << 1);

  n16 tilemap  = ppu.readVRAM_BG(Half, offset);
  latch.character = tilemap.bit(0,9);
  latch.hflip     = tilemap.bit(10);
  latch.vflip     = tilemap.bit(11);
  latch.palette   = tilemap.bit(12,15);
}

auto PPU::Background::linearFetchTileData() -> void {
  n3 px = fx;
  n3 py = fy;
  if(latch.hflip) px = ~px;
  if(latch.vflip) py = ~py;

  u32 tileOffset = ((latch.character << 5) + (py << 2)) << io.colorMode;
  u32 pixelOffset = px >> (1 - io.colorMode);
  u32 offset = (io.characterBase << 14) + tileOffset + pixelOffset;

  latch.data = ppu.readVRAM_BG(Half, offset);
}

auto PPU::Background::linear(u32 x, u32 y) -> void {
  if(x > 239) return;
  if(ppu.blank() || !io.enable[0]) return;

  if(x == 0) {
    if(!io.mosaic || (y % (1 + io.mosaicHeight)) == 0) {
      vmosaic = y;
    }
    fx = io.hoffset;
    fy = vmosaic + io.voffset;
  }

  n3 px = fx;
  n3 py = fy;

  if(x == 0 || px == 0) linearFetchTileMap();
  if(x == 0) {
    linearFetchTileData();
  } else if(io.colorMode == 0 && (px & 3) == 0) {
    linearFetchTileData();
  } else if(io.colorMode == 1 && (px & 1) == 0) {
    linearFetchTileData();
  }

  if(latch.hflip) px = ~px;

  n16 color = latch.data;
  if(io.colorMode == 0) {
    color >>= (px & 3) * 4;
    color = (n4)color;
  } else {
    color >>= (px & 1) * 8;
    color = (n8)color;
  }

  if(color) {
    if(io.colorMode == 0) color |= latch.palette << 4;
    output[x].enable = true;
    output[x].priority = io.priority;
    output[x].color = color;
  }

  fx++;
}

auto PPU::Background::affineFetchTileMap(u32 x, u32 y) -> void {
  if(x > 239) return;
  if(ppu.blank() || !io.enable[0]) return;

  if(x == 0) {
    if(!io.mosaic || (y % (1 + io.mosaicHeight)) == 0) {
      hmosaic = io.lx;
      vmosaic = io.ly;
    }
    fx = hmosaic;
    fy = vmosaic;
  }

  affine.screenSize = 16 << io.screenSize;
  affine.screenWrap = (1 << (io.affineWrap ? 7 + io.screenSize : 20)) - 1;

  affine.cx = (fx >> 8) & affine.screenWrap;
  affine.cy = (fy >> 8) & affine.screenWrap;

  affine.tx = affine.cx >> 3;
  affine.ty = affine.cy >> 3;

  affine.character = ppu.readVRAM_BG(Byte, (io.screenBase << 11) + affine.ty * affine.screenSize + affine.tx);
}

auto PPU::Background::affineFetchTileData(u32 x, u32 y) -> void {
  if(x > 239) return;
  if(ppu.blank() || !io.enable[0]) return;

  if(affine.tx < affine.screenSize && affine.ty < affine.screenSize) {
    n3 px = affine.cx;
    n3 py = affine.cy;
    if(n8 color = ppu.readVRAM_BG(Byte, (io.characterBase << 14) + (affine.character << 6) + (py << 3) + px)) {
      output[x].enable = true;
      output[x].priority = io.priority;
      output[x].color = color;
    }
  }

  fx += io.pa;
  fy += io.pc;

  if(x == 239) {
    io.lx += io.pb;
    io.ly += io.pd;
  }
}

auto PPU::Background::bitmap(u32 x, u32 y) -> void {
  if(x > 239) return;
  if(ppu.blank() || !io.enable[0]) return;

  if(x == 0) {
    if(!io.mosaic || (y % (1 + io.mosaicHeight)) == 0) {
      hmosaic = io.lx;
      vmosaic = io.ly;
    }
    fx = hmosaic;
    fy = vmosaic;
  }

  n1  depth = io.mode != 4;  //0 = 8-bit (mode 4); 1 = 15-bit (mode 3, mode 5)
  u32 width  = io.mode == 5 ? 160 : 240;
  u32 height = io.mode == 5 ? 128 : 160;
  u32 mode   = depth ? Half : Byte;

  u32 baseAddress = io.mode == 3 ? 0 : 0xa000 * io.frame;

  u32 px = fx >> 8;
  u32 py = fy >> 8;

  if(px < width && py < height) {
    u32 offset = py * width + px;
    n15 color = ppu.readVRAM_BG(mode, baseAddress + (offset << depth));

    if(depth || color) {  //8bpp color 0 is transparent; 15bpp color is always opaque
      if(depth) output[x].directColor = true;
      output[x].enable = true;
      output[x].priority = io.priority;
      output[x].color = color;
    }
  }

  fx += io.pa;
  fy += io.pc;

  if(x == 239) {
    io.lx += io.pb;
    io.ly += io.pd;
  }
}

auto PPU::Background::power(u32 id) -> void {
  this->id = id;

  io = {};
  latch = {};
  for(auto& pixel : output) pixel = {};
  mosaic = {};
  mosaicOffset = 0;
  hmosaic = 0;
  vmosaic = 0;
  fx = 0;
  fy = 0;
}
