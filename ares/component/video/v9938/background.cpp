auto V9938::Background::setup(n8 y) -> void {
  latch.vscroll = io.vscroll;
}

auto V9938::Background::run(n8 x, n8 y) -> void {
  output = {};
  switch(self.videoMode()) {
  case 0b00000: return graphic1(x, y);
  case 0b00001: return text1(x, y);
  case 0b00010: return multicolor(x, y);
  case 0b00100: return graphic2(x, y);
  case 0b01000: return graphic3(x, y);
  case 0b01001: return text2(x, y);
  case 0b01100: return graphic4(x, y);
  case 0b10000: return graphic5(x, y);
  case 0b10100: return graphic6(x, y);
  case 0b11100: return graphic7(x, y);
  }
}

auto V9938::Background::text1(n8 hoffset, n8 voffset) -> void {
  debug(unimplemented, "[V9938::Background::text1]");
}

auto V9938::Background::text2(n8 hoffset, n8 voffset) -> void {
  debug(unimplemented, "[V9938::Background::text2]");
}

auto V9938::Background::graphic1(n8 hoffset, n8 voffset) -> void {
  n14 patternLayout;
  patternLayout.bit( 0, 4) = hoffset.bit(3,7);
  patternLayout.bit( 5, 9) = voffset.bit(3,7);
  patternLayout.bit(10,13) = io.nameTableAddress.bit(10,13);
  n8 pattern = self.vram.read(patternLayout);

  n14 patternGenerator;
  patternGenerator.bit( 0, 2) = voffset.bit(0,2);
  patternGenerator.bit( 3,10) = pattern;
  patternGenerator.bit(11,13) = io.patternTableAddress.bit(11,13);

  n14 colorAddress;  //d5 = 0
  colorAddress.bit(0, 4) = pattern.bit(3,7);
  colorAddress.bit(6,13) = io.colorTableAddress.bit(6,13);

  n8 color = self.vram.read(colorAddress);
  if(self.vram.read(patternGenerator).bit(~hoffset & 7)) color >>= 4;
  output.color = color.bit(0,3);
}

auto V9938::Background::graphic2(n8 hoffset, n8 voffset) -> void {
  n14 patternLayout;
  patternLayout.bit( 0, 4) = hoffset.bit(3,7);
  patternLayout.bit( 5, 9) = voffset.bit(3,7);
  patternLayout.bit(10,13) = io.nameTableAddress.bit(10,13);
  n8 pattern = self.vram.read(patternLayout);

  n14 patternGenerator;
  patternGenerator.bit(0, 2) = voffset.bit(0,2);
  patternGenerator.bit(3,10) = pattern;
  if(voffset >=  64 && voffset <= 127) patternGenerator.bit(11) = io.patternTableAddress.bit(11);
  if(voffset >= 128 && voffset <= 191) patternGenerator.bit(12) = io.patternTableAddress.bit(12);

  n14 colorAddress = patternGenerator;
  patternGenerator.bit(13) = io.patternTableAddress.bit(13);
  colorAddress.bit(13) = io.colorTableAddress.bit(13);

  n8 colorMask = io.colorTableAddress.bit(6,12) << 1 | 1;
  n8 color = self.vram.read(colorAddress);
  if(self.vram.read(patternGenerator).bit(~hoffset & 7)) color >>= 4;
  output.color = color.bit(0,3);
}

auto V9938::Background::graphic3(n8 hoffset, n8 voffset) -> void {
  //graphic mode 3 is identical to graphic mode 2:
  //the difference is only that mode 2 uses sprite mode 1, and mode 3 uses sprite mode 2
  return graphic2(hoffset, voffset);
}

auto V9938::Background::graphic4(n8 hoffset, n8 voffset) -> void {
  n17 address = io.nameTableAddress & 0x18000;
  address += voffset << 7;
  address += hoffset >> 1;
  auto data = self.vram.read(address);
  auto shift = !hoffset.bit(0) ? 4 : 0;
  output.color = n4(data >> shift);
}

auto V9938::Background::graphic5(n8 hoffset, n8 voffset) -> void {
  debug(unimplemented, "[V9938::Background::graphic5]");
}

auto V9938::Background::graphic6(n8 hoffset, n8 voffset) -> void {
  debug(unimplemented, "[V9938::Background::graphic6]");
}

auto V9938::Background::graphic7(n8 hoffset, n8 voffset) -> void {
  debug(unimplemented, "[V9938::Background::graphic7]");
}

auto V9938::Background::multicolor(n8 hoffset, n8 voffset) -> void {
  debug(unimplemented, "[V9938::Background::multicolor]");
}

auto V9938::Background::power() -> void {
  io = {};
  latch = {};
  output = {};
}
