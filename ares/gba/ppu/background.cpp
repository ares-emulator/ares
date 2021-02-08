//I/O settings shared by all background layers
n3 PPU::Background::IO::mode;
n1 PPU::Background::IO::frame;
n5 PPU::Background::IO::mosaicWidth;
n5 PPU::Background::IO::mosaicHeight;

auto PPU::Background::scanline(u32 y) -> void {
  mosaicOffset = 0;
}

auto PPU::Background::run(u32 x, u32 y) -> void {
  output = {};
  if(ppu.blank() || !io.enable) {
    mosaic = {};
    return;
  }

  switch(id) {
  case PPU::BG0:
    if(io.mode <= 1) { linear(x, y); break; }
    break;

  case PPU::BG1:
    if(io.mode <= 1) { linear(x, y); break; }
    break;

  case PPU::BG2:
    if(io.mode == 0) { linear(x, y); break; }
    if(io.mode <= 2) { affine(x, y); break; }
    if(io.mode <= 5) { bitmap(x, y); break; }
    break;

  case PPU::BG3:
    if(io.mode == 0) { linear(x, y); break; }
    if(io.mode == 2) { affine(x, y); break; }
    break;
  }

  //horizontal mosaic
  if(!io.mosaic || ++mosaicOffset >= 1 + io.mosaicWidth) {
    mosaicOffset = 0;
    mosaic = output;
  }
}

auto PPU::Background::linear(u32 x, u32 y) -> void {
  if(x == 0) {
    if(!io.mosaic || (y % (1 + io.mosaicHeight)) == 0) {
      vmosaic = y;
    }
    fx = io.hoffset;
    fy = vmosaic + io.voffset;
  }

  n3 px = fx;
  n3 py = fy;

  if(x == 0 || px == 0) {
    n6 tx = fx >> 3;
    n6 ty = fy >> 3;

    u32 offset = (ty & 31) << 5 | (tx & 31);
    if(io.screenSize.bit(0) && (tx & 32)) offset += 32 << 5;
    if(io.screenSize.bit(1) && (ty & 32)) offset += 32 << 5 + io.screenSize.bit(0);
    offset = (io.screenBase << 11) + (offset << 1);

    n16 tilemap  = ppu.readVRAM(Half, offset);
    latch.character = tilemap.bit(0,9);
    latch.hflip     = tilemap.bit(10);
    latch.vflip     = tilemap.bit(11);
    latch.palette   = tilemap.bit(12,15);
  }

  if(latch.hflip) px = ~px;
  if(latch.vflip) py = ~py;

  if(io.colorMode == 0) {
    u32 offset = (io.characterBase << 14) + (latch.character << 5) + (py << 2) + (px >> 1);
    if(n4 color = ppu.readVRAM(Byte, offset) >> (px & 1 ? 4 : 0)) {
      output.enable = true;
      output.priority = io.priority;
      output.color = ppu.pram[latch.palette << 4 | color];
    }
  } else {
    u32 offset = (io.characterBase << 14) + (latch.character << 6) + (py << 3) + (px);
    if(n8 color = ppu.readVRAM(Byte, offset)) {
      output.enable = true;
      output.priority = io.priority;
      output.color = ppu.pram[color];
    }
  }

  fx++;
}

auto PPU::Background::affine(u32 x, u32 y) -> void {
  if(x == 0) {
    if(!io.mosaic || (y % (1 + io.mosaicHeight)) == 0) {
      hmosaic = io.lx;
      vmosaic = io.ly;
    }
    fx = hmosaic;
    fy = vmosaic;
  }

  u32 screenSize = 16 << io.screenSize;
  u32 screenWrap = (1 << (io.affineWrap ? 7 + io.screenSize : 20)) - 1;

  u32 cx = (fx >> 8) & screenWrap;
  u32 cy = (fy >> 8) & screenWrap;

  u32 tx = cx >> 3;
  u32 ty = cy >> 3;

  n3 px = cx;
  n3 py = cy;

  if(tx < screenSize && ty < screenSize) {
    n8 character = ppu.vram[(io.screenBase << 11) + ty * screenSize + tx];
    if(n8 color = ppu.vram[(io.characterBase << 14) + (character << 6) + (py << 3) + px]) {
      output.enable = true;
      output.priority = io.priority;
      output.color = ppu.pram[color];
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
    n15 color = ppu.readVRAM(mode, baseAddress + (offset << depth));

    if(depth || color) {  //8bpp color 0 is transparent; 15bpp color is always opaque
      if(depth == 0) color = ppu.pram[color];
      output.enable = true;
      output.priority = io.priority;
      output.color = color;
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
  output = {};
  mosaic = {};
  mosaicOffset = 0;
  hmosaic = 0;
  vmosaic = 0;
  fx = 0;
  fy = 0;
}
