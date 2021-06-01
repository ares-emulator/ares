auto VDP::Background::setup(n9 voffset) -> void {
  if(!self.displayEnable()) return;
  latch.nameTableAddress = io.nameTableAddress;
  latch.hscroll = io.hscroll;
  latch.vscroll = io.vscroll;
}

auto VDP::Background::run(n8 hoffset, n9 voffset) -> void {
  output = {};
  if(!self.displayEnable()) return;
  switch(self.videoMode()) {
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

auto VDP::Background::graphics1(n8 hoffset, n9 voffset) -> void {
  n14 nameTableAddress;
  nameTableAddress.bit( 0, 4) = hoffset.bit(3,7);
  nameTableAddress.bit( 5, 9) = voffset.bit(3,7);
  nameTableAddress.bit(10,13) = latch.nameTableAddress;
  n8 pattern = self.vram[nameTableAddress];

  n14 patternAddress;
  patternAddress.bit( 0, 2) = voffset.bit(0,2);
  patternAddress.bit( 3,10) = pattern;
  patternAddress.bit(11,13) = io.patternTableAddress;

  n14 colorAddress;  //d5 = 0
  colorAddress.bit(0, 4) = pattern.bit(3,7);
  colorAddress.bit(6,13) = io.colorTableAddress;

  n8 color = self.vram[colorAddress];
  n3 index = hoffset ^ 7;
  if(!self.vram[patternAddress].bit(index)) {
    output.color = color.bit(0,3);
  } else {
    output.color = color.bit(4,7);
  }
}

auto VDP::Background::graphics2(n8 hoffset, n9 voffset) -> void {
  n14 nameTableAddress;
  nameTableAddress.bit( 0, 4) = hoffset.bit(3,7);
  nameTableAddress.bit( 5, 9) = voffset.bit(3,7);
  nameTableAddress.bit(10,13) = latch.nameTableAddress;
  n8 pattern = self.vram[nameTableAddress];

  n14 patternAddress;
  patternAddress.bit(0, 2) = voffset.bit(0,2);
  patternAddress.bit(3,10) = pattern;
  if(voffset >=  64 && voffset <= 127) patternAddress.bit(11) = io.patternTableAddress.bit(0);
  if(voffset >= 128 && voffset <= 191) patternAddress.bit(12) = io.patternTableAddress.bit(1);
  n14 colorAddress = patternAddress;
  patternAddress.bit(13) = io.patternTableAddress.bit(2);
  colorAddress.bit(13) = io.colorTableAddress.bit(7);

  n8 colorMask = io.colorTableAddress.bit(0,6) << 1 | 1;
  n8 color = self.vram[colorAddress];
  n3 index = hoffset ^ 7;
  if(!self.vram[patternAddress].bit(index)) {
    output.color = color.bit(0,3);
  } else {
    output.color = color.bit(4,7);
  }
}

auto VDP::Background::graphics3(n8 hoffset, n9 voffset, u32 vlines) -> void {
  if(hoffset < latch.hscroll.bit(0,2)) return;

  if(!io.hscrollLock || voffset >=  16) hoffset -= latch.hscroll;
  if(!io.vscrollLock || hoffset <= 191) voffset += latch.vscroll;

  n14 nameTableAddress;
  if(vlines == 192) {
    voffset %= 224;
    nameTableAddress  = latch.nameTableAddress >> 1 << 11;
    nameTableAddress += voffset >> 3 << 6;
    nameTableAddress += hoffset >> 3 << 1;
    if(self.revision->value() == 1) {
      //SMS1 quirk: bit 0 of name table base address acts as a mask
      nameTableAddress.bit(10) &= latch.nameTableAddress.bit(0);
    }
  } else {
    voffset %= 256;
    nameTableAddress  = latch.nameTableAddress >> 2 << 12 | 0x0700;
    nameTableAddress += voffset >> 3 << 6;
    nameTableAddress += hoffset >> 3 << 1;
  }

  n16 pattern;
  pattern.byte(0) = self.vram[nameTableAddress | 0];
  pattern.byte(1) = self.vram[nameTableAddress | 1];

  if(pattern.bit( 9)) hoffset ^= 7;  //hflip
  if(pattern.bit(10)) voffset ^= 7;  //vflip
  output.palette  = pattern.bit(11);
  output.priority = pattern.bit(12);

  n14 patternAddress;
  patternAddress.bit(2, 4) = voffset.bit(0,2);
  patternAddress.bit(5,13) = pattern.bit(0,8);

  n3 index = hoffset ^ 7;
  output.color.bit(0) = self.vram[patternAddress | 0].bit(index);
  output.color.bit(1) = self.vram[patternAddress | 1].bit(index);
  output.color.bit(2) = self.vram[patternAddress | 2].bit(index);
  output.color.bit(3) = self.vram[patternAddress | 3].bit(index);

  if(output.color == 0) output.priority = 0;
}

auto VDP::Background::power() -> void {
  io = {};
  latch = {};
  output = {};
}
