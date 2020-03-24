auto TMS9918::background(uint8 hoffset, uint8 voffset) -> void {
  if(!io.displayEnable) {
    output.color = io.colorBackground;
    return;
  }

  switch(io.videoMode) {
  case 0: return graphics1(hoffset, voffset);
//case 1: return text1(hoffset, voffset);
//case 2: return multicolor(hoffset, voffset);
  case 4: return graphics2(hoffset, voffset);
  default: output.color = 8; return;  //medium red color to identify unimplemented modes
  }
}

auto TMS9918::text1(uint8 hoffset, uint8 voffset) -> void {
}

auto TMS9918::graphics1(uint8 hoffset, uint8 voffset) -> void {
  uint14 nameAddress;
  nameAddress.bit( 0, 4) = hoffset.bit(3,7);
  nameAddress.bit( 5, 9) = voffset.bit(3,7);
  nameAddress.bit(10,13) = io.nameTableAddress;

  uint8 pattern = vram.read(nameAddress);

  uint14 patternAddress;
  patternAddress.bit( 0, 2) = voffset.bit(0,2);
  patternAddress.bit( 3,10) = pattern;
  patternAddress.bit(11,13) = io.patternTableAddress;

  uint14 colorAddress;  //d5 = 0
  colorAddress.bit(0, 4) = pattern.bit(3,7);
  colorAddress.bit(6,13) = io.colorTableAddress;

  uint8 color = vram.read(colorAddress);
  uint3 index = hoffset ^ 7;
  if(!vram.read(patternAddress).bit(index)) {
    output.color = color.bit(0,3);
  } else {
    output.color = color.bit(4,7);
  }
}

auto TMS9918::graphics2(uint8 hoffset, uint8 voffset) -> void {
  uint14 nameAddress;
  nameAddress.bit( 0, 4) = hoffset.bit(3,7);
  nameAddress.bit( 5, 9) = voffset.bit(3,7);
  nameAddress.bit(10,13) = io.nameTableAddress;

  uint8 pattern = vram.read(nameAddress);

  uint14 patternAddress;
  patternAddress.bit(0, 2) = voffset.bit(0,2);
  patternAddress.bit(3,10) = pattern;
  if(voffset >=  64 && voffset <= 127) patternAddress.bit(11) = io.patternTableAddress.bit(0);
  if(voffset >= 128 && voffset <= 191) patternAddress.bit(12) = io.patternTableAddress.bit(1);
  uint14 colorAddress = patternAddress;
  patternAddress.bit(13) = io.patternTableAddress.bit(2);
  colorAddress.bit(13) = io.colorTableAddress.bit(7);

  uint8 colorMask = io.colorTableAddress.bit(0,6) << 1 | 1;
  uint8 color = vram.read(colorAddress);
  uint3 index = hoffset ^ 7;
  if(!vram.read(patternAddress).bit(index)) {
    output.color = color.bit(0,3);
  } else {
    output.color = color.bit(4,7);
  }
  if(!output.color) {
    output.color = io.colorBackground;
  }
}

auto TMS9918::multicolor(uint8 hoffset, uint8 voffset) -> void {
}
