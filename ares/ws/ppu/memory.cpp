auto PPU::fetch(n10 tile, n3 x, n3 y) -> n4 {
  n4 color;

  if(depth() == 2) tile &= 0x1ff;
  if(depth() == 4) tile &= 0x3ff;

  if(planar() && depth() == 2) {
    n16 data = iram.read16(0x2000 + (tile << 4) + (y << 1));
    color.bit(0) = data.bit( 7 - x);
    color.bit(1) = data.bit(15 - x);
  }

  if(planar() && depth() == 4) {
    n32 data = iram.read32(0x4000 + (tile << 5) + (y << 2));
    color.bit(0) = data.bit( 7 - x);
    color.bit(1) = data.bit(15 - x);
    color.bit(2) = data.bit(23 - x);
    color.bit(3) = data.bit(31 - x);
  }

  if(packed() && depth() == 2) {
    n8 data = iram.read8(0x2000 + (tile << 4) + (y << 1) + (x >> 2));
    color.bit(0,1) = data >> 6 - (x.bit(0,1) << 1);
  }

  if(packed() && depth() == 4) {
    n8 data = iram.read8(0x4000 + (tile << 5) + (y << 2) + (x >> 1));
    color.bit(0,3) = data >> 4 - (x.bit(0) << 2);
  }

  return color;
}

auto PPU::backdrop(n8 color) -> n12 {
  if(grayscale()) {
    n4 luma = 15 - pram.pool[color & 7];
    return luma << 0 | luma << 4 | luma << 8;
  } else {
    return iram.read16(0xfe00 + (color << 1));
  }
}

auto PPU::palette(n4 palette, n4 color) -> n12 {
  if(grayscale()) {
    n3 pool = pram.palette[palette].color[color & 3];
    n4 luma = 15 - pram.pool[pool];
    return luma << 0 | luma << 4 | luma << 8;
  } else {
    return iram.read16(0xfe00 + (palette << 5) + (color << 1));
  }
}

auto PPU::opaque(n4 palette, n4 color) -> n1 {
  if(color) return true;
  if(depth() == 2 && !palette.bit(2)) return true;
  return false;
}
