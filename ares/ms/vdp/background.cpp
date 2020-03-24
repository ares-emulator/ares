auto VDP::Background::run(uint8 hoffset, uint9 voffset) -> void {
  output = {};
  switch(vdp.io.mode) {
  case 0b0000: return graphics1(hoffset, voffset);
  case 0b0001: return;
  case 0b0010: return graphics2(hoffset, voffset);
  case 0b0011: return;
  case 0b0100: return;
  case 0b0101: return;
  case 0b0110: return;
  case 0b0111: return;
  case 0b1000: return graphics3(hoffset, voffset, 192);
  case 0b1001: return;
  case 0b1010: return graphics3(hoffset, voffset, 192);
  case 0b1011: return graphics3(hoffset, voffset, 224);
  case 0b1100: return graphics3(hoffset, voffset, 192);
  case 0b1101: return;
  case 0b1110: return graphics3(hoffset, voffset, 240);
  case 0b1111: return graphics3(hoffset, voffset, 192);
  }
}

auto VDP::Background::graphics1(uint8 hoffset, uint9 voffset) -> void {
  uint14 nameTableAddress;
  nameTableAddress.bit( 0, 4) = hoffset.bit(3,7);
  nameTableAddress.bit( 5, 9) = voffset.bit(3,7);
  nameTableAddress.bit(10,13) = vdp.io.nameTableAddress;
  uint8 pattern = vdp.vram[nameTableAddress];

  uint14 patternAddress;
  patternAddress.bit( 0, 2) = voffset.bit(0,2);
  patternAddress.bit( 3,10) = pattern;
  patternAddress.bit(11,13) = vdp.io.patternTableAddress;

  uint14 colorAddress;  //d5 = 0
  colorAddress.bit(0, 4) = pattern.bit(3,7);
  colorAddress.bit(6,13) = vdp.io.colorTableAddress;

  uint8 color = vdp.vram[colorAddress];
  uint3 index = hoffset ^ 7;
  if(!vdp.vram[patternAddress].bit(index)) {
    output.color = color.bit(0,3);
  } else {
    output.color = color.bit(4,7);
  }
}

auto VDP::Background::graphics2(uint8 hoffset, uint9 voffset) -> void {
  uint14 nameTableAddress;
  nameTableAddress.bit( 0, 4) = hoffset.bit(3,7);
  nameTableAddress.bit( 5, 9) = voffset.bit(3,7);
  nameTableAddress.bit(10,13) = vdp.io.nameTableAddress;
  uint8 pattern = vdp.vram[nameTableAddress];

  uint14 patternAddress;
  patternAddress.bit(0, 2) = voffset.bit(0,2);
  patternAddress.bit(3,10) = pattern;
  if(voffset >=  64 && voffset <= 127) patternAddress.bit(11) = vdp.io.patternTableAddress.bit(0);
  if(voffset >= 128 && voffset <= 191) patternAddress.bit(12) = vdp.io.patternTableAddress.bit(1);
  uint14 colorAddress = patternAddress;
  patternAddress.bit(13) = vdp.io.patternTableAddress.bit(2);
  colorAddress.bit(13) = vdp.io.colorTableAddress.bit(7);

  uint8 colorMask = vdp.io.colorTableAddress.bit(0,6) << 1 | 1;
  uint8 color = vdp.vram[colorAddress];
  uint3 index = hoffset ^ 7;
  if(!vdp.vram[patternAddress].bit(index)) {
    output.color = color.bit(0,3);
  } else {
    output.color = color.bit(4,7);
  }
}

auto VDP::Background::graphics3(uint8 hoffset, uint9 voffset, uint vlines) -> void {
  if(hoffset < vdp.io.hscroll.bit(0,2)) return;

  if(!vdp.io.horizontalScrollLock || voffset >=  16) hoffset -= vdp.io.hscroll;
  if(!vdp.io.verticalScrollLock   || hoffset <= 191) voffset += vdp.io.vscroll;

  uint14 nameTableAddress;
  if(vlines == 192) {
    if(voffset >= 224) voffset -= 224;
    nameTableAddress.bit( 1, 5) = hoffset.bit(3,7);
    nameTableAddress.bit( 6,10) = voffset.bit(3,7);
    nameTableAddress.bit(11,13) = vdp.io.nameTableAddress.bit(1,3);
  } else {
    voffset += 224;
    nameTableAddress.bit( 1, 5) = hoffset.bit(3,7);
    nameTableAddress.bit( 6,11) = voffset.bit(3,8);
    nameTableAddress.bit(12,13) = vdp.io.nameTableAddress.bit(2,3);
  }

  uint16 pattern;
  pattern.byte(0) = vdp.vram[nameTableAddress | 0];
  pattern.byte(1) = vdp.vram[nameTableAddress | 1];

  if(pattern.bit( 9)) hoffset ^= 7;  //hflip
  if(pattern.bit(10)) voffset ^= 7;  //vflip
  output.palette  = pattern.bit(11);
  output.priority = pattern.bit(12);

  uint14 patternAddress;
  patternAddress.bit(2, 4) = voffset.bit(0,2);
  patternAddress.bit(5,13) = pattern.bit(0,8);

  uint3 index = hoffset ^ 7;
  output.color.bit(0) = vdp.vram[patternAddress | 0].bit(index);
  output.color.bit(1) = vdp.vram[patternAddress | 1].bit(index);
  output.color.bit(2) = vdp.vram[patternAddress | 2].bit(index);
  output.color.bit(3) = vdp.vram[patternAddress | 3].bit(index);

  if(output.color == 0) output.priority = 0;
}

auto VDP::Background::power() -> void {
  output = {};
}
