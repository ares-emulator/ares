auto PPU::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(screen1.enable);
  s(screen1.mapBase);
  s(screen1.hscroll);
  s(screen1.vscroll);
  s(screen1.output.valid);
  s(screen1.output.color);

  s(screen2.enable);
  s(screen2.mapBase);
  s(screen2.hscroll);
  s(screen2.vscroll);
  s(screen2.output.valid);
  s(screen2.output.color);
  s(screen2.window.enable);
  s(screen2.window.invert);
  s(screen2.window.x0);
  s(screen2.window.x1);
  s(screen2.window.y0);
  s(screen2.window.y1);

  s(sprite.enable);
  s(sprite.oamBase);
  s(sprite.first);
  s(sprite.count);
  s(sprite.valid);
  s(sprite.output.valid);
  s(sprite.output.color);
  s(sprite.window.enable);
  s(sprite.window.x0);
  s(sprite.window.x1);
  s(sprite.window.y0);
  s(sprite.window.y1);
  for(auto& line : sprite.oam) s(line);
  s(sprite.objects);

  s(dac.enable);
  s(dac.contrast);
  s(dac.unknown);
  s(dac.backdrop);

  s(pram.pool);
  for(auto& palette : pram.palette) s(palette.color);

  s(lcd.icon.sleeping);
  s(lcd.icon.orientation1);
  s(lcd.icon.orientation0);
  s(lcd.icon.auxiliary0);
  s(lcd.icon.auxiliary1);
  s(lcd.icon.auxiliary2);

  s(htimer.enable);
  s(htimer.repeat);
  s(htimer.frequency);
  s(htimer.counter);

  s(vtimer.enable);
  s(vtimer.repeat);
  s(vtimer.frequency);
  s(vtimer.counter);

  s(io.hcounter);
  s(io.vcounter);
  s(io.vsync);
  s(io.vtotal);
  s(io.vcompare);
  s(io.field);
  s(io.orientation);
}
