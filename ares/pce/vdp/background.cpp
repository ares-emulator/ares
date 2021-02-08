auto VDC::Background::scanline(u32 y) -> void {
  if(y == 0) {
    vcounter = vscroll;
  } else {
    vcounter++;
  }
  hoffset = hscroll;
  voffset = vcounter;
}

auto VDC::Background::run(u32 x, u32 y) -> void {
  color = 0;
  palette = 0;
  if(!enable) return;

  n8  tileX = hoffset >> 3 & width  - 1;
  n8  tileY = voffset >> 3 & height - 1;
  n16 attributes = vdc->vram.read(tileY * width + tileX);

  n16 patternAddress = attributes.bit(0,11) << 4 | (n3)voffset;
  n4  palette = attributes.bit(12,15);

  n16 d0 = 0, d1 = 0;
  if(latch.vramMode != 3) {
    d0 = vdc->vram.read(patternAddress + 0);
    d1 = vdc->vram.read(patternAddress + 8);
  }
  if(latch.vramMode == 3) {
    if(latch.characterMode == 0) d0 = vdc->vram.read(patternAddress + 0);
    if(latch.characterMode == 1) d0 = vdc->vram.read(patternAddress + 8);
  }

  n3 index = ~hoffset;
  n4 color;
  color.bit(0) = d0.bit(0 + index);
  color.bit(1) = d0.bit(8 + index);
  color.bit(2) = d1.bit(0 + index);
  color.bit(3) = d1.bit(8 + index);

  if(color) {
    this->color = color;
    this->palette = palette;
  }

  hoffset++;
}
