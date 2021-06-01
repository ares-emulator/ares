auto V9938::DAC::setup(n8 voffset) -> void {
  n9 vcenter = voffset + (!self.overscan() ? 10 : 0);  //centers output within buffer
  output = self.screen->pixels().data() + vcenter * 1024;
  if(self.interlace() && self.field()) output += 512;
}

auto V9938::DAC::run(n8 hoffset, n8 voffset) -> void {
  n4 color = self.io.colorBackground;
  if(io.enable) {
    if(self.background.output.color) color = self.background.output.color;
    if(self.sprite.output.color) color = self.sprite.output.color;
  }
  *output++ = self.pram[color];
  *output++ = self.pram[color];
}

auto V9938::DAC::power() -> void {
  //format: ggg-rrr-bbb (octal encoding)
  //the default palette is an approximation of the TMS9918 palette
  self.pram[ 0] = 0'000;
  self.pram[ 1] = 0'000;
  self.pram[ 2] = 0'611;
  self.pram[ 3] = 0'733;
  self.pram[ 4] = 0'117;
  self.pram[ 5] = 0'327;
  self.pram[ 6] = 0'151;
  self.pram[ 7] = 0'627;
  self.pram[ 8] = 0'171;
  self.pram[ 9] = 0'373;
  self.pram[10] = 0'661;
  self.pram[11] = 0'664;
  self.pram[12] = 0'411;
  self.pram[13] = 0'265;
  self.pram[14] = 0'555;
  self.pram[15] = 0'777;

  io = {};
  output = nullptr;
}
