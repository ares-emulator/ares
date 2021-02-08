auto PPU::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(this->s.field);
  s(this->s.vtime);
  s((u32&)this->s.pixel.source);
  s(this->s.pixel.color);

  s(l.backColor);
  s(l.screenOneEnable);
  s(l.screenOneMapBase);
  s(l.scrollOneX);
  s(l.scrollOneY);
  s(l.screenTwoEnable);
  s(l.screenTwoMapBase);
  s(l.scrollTwoX);
  s(l.scrollTwoY);
  s(l.screenTwoWindowEnable);
  s(l.screenTwoWindowInvert);
  s(l.screenTwoWindowX0);
  s(l.screenTwoWindowY0);
  s(l.screenTwoWindowX1);
  s(l.screenTwoWindowY1);
  s(l.spriteEnable);
  s(l.spriteWindowEnable);
  s(l.spriteWindowX0);
  s(l.spriteWindowY0);
  s(l.spriteWindowX1);
  s(l.spriteWindowY1);

  s(l.sprite);
  s(l.spriteCount);

  for(u32 n : range(2)) s(l.oam[n]);
  s(l.oamCount);

  s(l.orientation);

  s(r.screenOneEnable);
  s(r.screenTwoEnable);
  s(r.spriteEnable);
  s(r.spriteWindowEnable);
  s(r.screenTwoWindowInvert);
  s(r.screenTwoWindowEnable);
  s(r.backColor);
  s(r.lineCompare);
  s(r.spriteBase);
  s(r.spriteFirst);
  s(r.spriteCount);
  s(r.screenOneMapBase);
  s(r.screenTwoMapBase);
  s(r.screenTwoWindowX0);
  s(r.screenTwoWindowY0);
  s(r.screenTwoWindowX1);
  s(r.screenTwoWindowY1);
  s(r.spriteWindowX0);
  s(r.spriteWindowY0);
  s(r.spriteWindowX1);
  s(r.spriteWindowY1);
  s(r.scrollOneX);
  s(r.scrollOneY);
  s(r.scrollTwoX);
  s(r.scrollTwoY);
  s(r.lcdEnable);
  s(r.lcdContrast);
  s(r.lcdUnknown);
  s(r.icon.sleeping);
  s(r.icon.orientation1);
  s(r.icon.orientation0);
  s(r.icon.auxiliary0);
  s(r.icon.auxiliary1);
  s(r.icon.auxiliary2);
  s(r.vtotal);
  s(r.vsync);
  s(r.pool);
  for(u32 n : range(16)) s(r.palette[n].color);
  s(r.htimerEnable);
  s(r.htimerRepeat);
  s(r.vtimerEnable);
  s(r.vtimerRepeat);
  s(r.htimerFrequency);
  s(r.vtimerFrequency);
  s(r.htimerCounter);
  s(r.vtimerCounter);
}
