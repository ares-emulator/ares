auto VDC::Background::scanline(uint y) -> void {
  if(y == 0) {
    vcounter = vscroll;
  } else {
    vcounter++;
  }
  hoffset = hscroll;
  voffset = vcounter;
}

auto VDC::Background::run(uint x, uint y) -> void {
  color = 0;
  palette = 0;
  if(!enable) return;

  uint8 tileX = hoffset >> 3 & width  - 1;
  uint8 tileY = voffset >> 3 & height - 1;
  uint16 attributes = vdc->vram.read(tileY * width + tileX);

  uint16 patternAddress = attributes.bit(0,11) << 4 | (uint3)voffset;
  uint4 palette = attributes.bit(12,15);

  uint16 d0 = 0, d1 = 0;
  if(latch.vramMode != 3) {
    d0 = vdc->vram.read(patternAddress + 0);
    d1 = vdc->vram.read(patternAddress + 8);
  }
  if(latch.vramMode == 3) {
    if(latch.characterMode == 0) d0 = vdc->vram.read(patternAddress + 0);
    if(latch.characterMode == 1) d0 = vdc->vram.read(patternAddress + 8);
  }

  uint3 index = ~hoffset;
  uint4 color;
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
