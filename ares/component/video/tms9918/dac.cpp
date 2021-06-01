auto TMS9918::DAC::setup(n8 y) -> void {
  output = self.screen->pixels().data() + y * 256;
}

auto TMS9918::DAC::run(n8 x, n8 y) -> void {
  n6 color = io.colorBackground;
  if(io.displayEnable) {
    if(self.background.output.color) color = self.background.output.color;
    if(self.sprite.output.color) color = self.sprite.output.color;
  }
  output[x] = color;
}

auto TMS9918::DAC::power() -> void {
  output = nullptr;
}
