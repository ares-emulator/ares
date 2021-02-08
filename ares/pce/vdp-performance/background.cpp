auto VDC::Background::scanline(n16 y) -> void {
  if(y == 0) {
    vcounter = vscroll;
  } else {
    vcounter++;
  }
  hoffset = hscroll;
  voffset = vcounter;
}

auto VDC::Background::render(n16 y) -> void {
  if(!enable) return (void)memset(&output, 0, sizeof(output));

  for(u32 x = 0; x < vdp.vce.width();) {
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
    do {
      n4 color;
      color.bit(0) = d0.bit(0 + index);
      color.bit(1) = d0.bit(8 + index);
      color.bit(2) = d1.bit(0 + index);
      color.bit(3) = d1.bit(8 + index);
      index--;
      output[x++] = {color, palette};
    } while(++hoffset & 7);
  }
}
