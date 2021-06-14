auto PPU::DAC::scanline(n8 y) -> void {
  output = self.screen->pixels().data() + y * self.screen->width();
}

auto PPU::DAC::pixel(n8 x, n8 y) -> void {
  n12 color = self.backdrop(backdrop);

  if(self.lcd.enable) {
    if(screen1.output.valid) color = screen1.output.color;
    if(screen2.output.valid) color = screen2.output.color;
    if( sprite.output.valid) color =  sprite.output.color;
    if(Model::WonderSwanColor() && self.lcd.contrast == 1) {
      n4 b = color.bit(0, 3);
      n4 g = color.bit(4, 7);
      n4 r = color.bit(8,11);
      //this is just a rough approximation, not accurate to the actual effect.
      color.bit(0, 3) = min(15, b * 1.5);
      color.bit(4, 7) = min(15, g * 1.5);
      color.bit(8,11) = min(15, r * 1.5);
    }
  }

  output[x] = color;
}

auto PPU::DAC::power() -> void {
  backdrop = 0;
}
