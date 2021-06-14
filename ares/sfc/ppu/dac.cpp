auto PPU::DAC::scanline() -> void {
  line = nullptr;

  auto vcounter = self.vcounter();
  auto output = self.screen->pixels().data();
  if(!self.overscan()) vcounter += 8;
  if(vcounter < 240) {
    line = output + vcounter * 2 * 512;
    if(self.interlace() && self.field()) line += 512;
  }

  //the first hires pixel of each scanline is transparent
  //note: exact value initializations are not confirmed on hardware
  math.above.color = paletteColor(0);
  math.below.color = math.above.color;

  math.above.colorEnable = false;
  math.below.colorEnable = false;

  math.transparent = true;
  math.blendMode   = false;
  math.colorHalve  = io.colorHalve && !io.blendMode && math.above.colorEnable;
}

auto PPU::DAC::run() -> void {
  if(self.vcounter() == 0) return;

  bool hires      = self.io.pseudoHires || self.io.bgMode == 5 || self.io.bgMode == 6;
  auto belowColor = below(hires);
  auto aboveColor = above();

  if(!line) return;
  *line++ = self.io.displayBrightness << 15 | (hires ? belowColor : aboveColor);
  *line++ = self.io.displayBrightness << 15 | (aboveColor);
}

auto PPU::DAC::below(bool hires) -> n16 {
  if(self.io.displayDisable || (!self.io.overscan && self.vcounter() >= 225)) return 0;

  u32 priority = 0;
  if(bg1.output.below.priority) {
    priority = bg1.output.below.priority;
    if(io.directColor && (self.io.bgMode == 3 || self.io.bgMode == 4 || self.io.bgMode == 7)) {
      math.below.color = directColor(bg1.output.below.palette, bg1.output.below.paletteGroup);
    } else {
      math.below.color = paletteColor(bg1.output.below.palette);
    }
  }
  if(bg2.output.below.priority > priority) {
    priority = bg2.output.below.priority;
    math.below.color = paletteColor(bg2.output.below.palette);
  }
  if(bg3.output.below.priority > priority) {
    priority = bg3.output.below.priority;
    math.below.color = paletteColor(bg3.output.below.palette);
  }
  if(bg4.output.below.priority > priority) {
    priority = bg4.output.below.priority;
    math.below.color = paletteColor(bg4.output.below.palette);
  }
  if(obj.output.below.priority > priority) {
    priority = obj.output.below.priority;
    math.below.color = paletteColor(obj.output.below.palette);
  }
  if(math.transparent = (priority == 0)) math.below.color = paletteColor(0);

  if(!hires) return 0;
  if(!math.below.colorEnable) return math.above.colorEnable ? math.below.color : (n15)0;

  return blend(
    math.above.colorEnable ? math.below.color : (n15)0,
    math.blendMode ? math.above.color : fixedColor()
  );
}

auto PPU::DAC::above() -> n16 {
  if(self.io.displayDisable || (!self.io.overscan && self.vcounter() >= 225)) return 0;

  u32 priority = 0;
  if(bg1.output.above.priority) {
    priority = bg1.output.above.priority;
    if(io.directColor && (self.io.bgMode == 3 || self.io.bgMode == 4 || self.io.bgMode == 7)) {
      math.above.color = directColor(bg1.output.above.palette, bg1.output.above.paletteGroup);
    } else {
      math.above.color = paletteColor(bg1.output.above.palette);
    }
    math.below.colorEnable = io.bg1.colorEnable;
  }
  if(bg2.output.above.priority > priority) {
    priority = bg2.output.above.priority;
    math.above.color = paletteColor(bg2.output.above.palette);
    math.below.colorEnable = io.bg2.colorEnable;
  }
  if(bg3.output.above.priority > priority) {
    priority = bg3.output.above.priority;
    math.above.color = paletteColor(bg3.output.above.palette);
    math.below.colorEnable = io.bg3.colorEnable;
  }
  if(bg4.output.above.priority > priority) {
    priority = bg4.output.above.priority;
    math.above.color = paletteColor(bg4.output.above.palette);
    math.below.colorEnable = io.bg4.colorEnable;
  }
  if(obj.output.above.priority > priority) {
    priority = obj.output.above.priority;
    math.above.color = paletteColor(obj.output.above.palette);
    math.below.colorEnable = io.obj.colorEnable && obj.output.above.palette >= 192;
  }
  if(priority == 0) {
    math.above.color = paletteColor(0);
    math.below.colorEnable = io.back.colorEnable;
  }

  if(!window.output.below.colorEnable) math.below.colorEnable = false;
  math.above.colorEnable = window.output.above.colorEnable;
  if(!math.below.colorEnable) return math.above.colorEnable ? math.above.color : (n15)0;

  if(io.blendMode && math.transparent) {
    math.blendMode  = false;
    math.colorHalve = false;
  } else {
    math.blendMode  = io.blendMode;
    math.colorHalve = io.colorHalve && math.above.colorEnable;
  }

  return blend(
    math.above.colorEnable ? math.above.color : (n15)0,
    math.blendMode ? math.below.color : fixedColor()
  );
}

auto PPU::DAC::blend(u32 x, u32 y) const -> n15 {
  if(!io.colorMode) {  //add
    if(!math.colorHalve) {
      u32 sum = x + y;
      u32 carry = (sum - ((x ^ y) & 0x0421)) & 0x8420;
      return (sum - carry) | (carry - (carry >> 5));
    } else {
      return (x + y - ((x ^ y) & 0x0421)) >> 1;
    }
  } else {  //sub
    u32 diff = x - y + 0x8420;
    u32 borrow = (diff - ((x ^ y) & 0x8420)) & 0x8420;
    if(!math.colorHalve) {
      return   (diff - borrow) & (borrow - (borrow >> 5));
    } else {
      return (((diff - borrow) & (borrow - (borrow >> 5))) & 0x7bde) >> 1;
    }
  }
}

inline auto PPU::DAC::paletteColor(n8 palette) const -> n15 {
  self.latch.cgramAddress = palette;
  return cgram[palette];
}

inline auto PPU::DAC::directColor(n8 palette, n3 paletteGroup) const -> n15 {
  //palette = -------- BBGGGRRR
  //group   = -------- -----bgr
  //output  = 0BBb00GG Gg0RRRr0
  return (palette << 7 & 0x6000) + (paletteGroup << 10 & 0x1000)
       + (palette << 4 & 0x0380) + (paletteGroup <<  5 & 0x0040)
       + (palette << 2 & 0x001c) + (paletteGroup <<  1 & 0x0002);
}

inline auto PPU::DAC::fixedColor() const -> n15 {
  return io.colorRed << 0 | io.colorGreen << 5 | io.colorBlue << 10;
}

auto PPU::DAC::power() -> void {
  io.blendMode = random();
  io.directColor = random();
  io.colorMode = random();
  io.colorHalve = random();
  io.bg1.colorEnable = random();
  io.bg2.colorEnable = random();
  io.bg3.colorEnable = random();
  io.bg4.colorEnable = random();
  io.obj.colorEnable = random();
  io.back.colorEnable = random();
  io.colorBlue = random();
  io.colorGreen = random();
  io.colorRed = random();
}
