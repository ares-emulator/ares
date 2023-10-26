auto TMS9918::DAC::setup(n9 y) -> void {
  y = (y + 27) % 243;
  output = self.screen->pixels().data() + y * 284;
  for (u32 x : range(284)) output[x] = io.colorBackground;
}

auto TMS9918::DAC::run(n8 x, n8 y) -> void {
  n6 color = io.colorBackground;
  if(io.displayEnable) {
    if(self.background.output.color) color = self.background.output.color;
    if(self.sprite.output.color) color = self.sprite.output.color;
  }
  output[(x + 13) % 284] = color;
}

auto TMS9918::DAC::power() -> void {
  output = nullptr;
}
