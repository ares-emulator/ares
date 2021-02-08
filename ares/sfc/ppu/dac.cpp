auto PPU::DAC::scanline() -> void {
  line = nullptr;

  auto vcounter = ppu.vcounter();
  auto output = ppu.screen->pixels().data();
  if(!ppu.self.overscan) vcounter += 8;
  if(vcounter < 240) {
    line = output + vcounter * 2 * 512;
    if(ppu.self.interlace && ppu.field()) line += 512;
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
  if(ppu.vcounter() == 0) return;

  bool hires      = ppu.io.pseudoHires || ppu.io.bgMode == 5 || ppu.io.bgMode == 6;
  auto belowColor = below(hires);
  auto aboveColor = above();

  if(!line) return;
  *line++ = ppu.io.displayBrightness << 15 | (hires ? belowColor : aboveColor);
  *line++ = ppu.io.displayBrightness << 15 | (aboveColor);
}

auto PPU::DAC::below(bool hires) -> n16 {
  if(ppu.io.displayDisable || (!ppu.io.overscan && ppu.vcounter() >= 225)) return 0;

  u32 priority = 0;
  if(ppu.bg1.output.below.priority) {
    priority = ppu.bg1.output.below.priority;
    if(io.directColor && (ppu.io.bgMode == 3 || ppu.io.bgMode == 4 || ppu.io.bgMode == 7)) {
      math.below.color = directColor(ppu.bg1.output.below.palette, ppu.bg1.output.below.paletteGroup);
    } else {
      math.below.color = paletteColor(ppu.bg1.output.below.palette);
    }
  }
  if(ppu.bg2.output.below.priority > priority) {
    priority = ppu.bg2.output.below.priority;
    math.below.color = paletteColor(ppu.bg2.output.below.palette);
  }
  if(ppu.bg3.output.below.priority > priority) {
    priority = ppu.bg3.output.below.priority;
    math.below.color = paletteColor(ppu.bg3.output.below.palette);
  }
  if(ppu.bg4.output.below.priority > priority) {
    priority = ppu.bg4.output.below.priority;
    math.below.color = paletteColor(ppu.bg4.output.below.palette);
  }
  if(ppu.obj.output.below.priority > priority) {
    priority = ppu.obj.output.below.priority;
    math.below.color = paletteColor(ppu.obj.output.below.palette);
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
  if(ppu.io.displayDisable || (!ppu.io.overscan && ppu.vcounter() >= 225)) return 0;

  u32 priority = 0;
  if(ppu.bg1.output.above.priority) {
    priority = ppu.bg1.output.above.priority;
    if(io.directColor && (ppu.io.bgMode == 3 || ppu.io.bgMode == 4 || ppu.io.bgMode == 7)) {
      math.above.color = directColor(ppu.bg1.output.above.palette, ppu.bg1.output.above.paletteGroup);
    } else {
      math.above.color = paletteColor(ppu.bg1.output.above.palette);
    }
    math.below.colorEnable = io.bg1.colorEnable;
  }
  if(ppu.bg2.output.above.priority > priority) {
    priority = ppu.bg2.output.above.priority;
    math.above.color = paletteColor(ppu.bg2.output.above.palette);
    math.below.colorEnable = io.bg2.colorEnable;
  }
  if(ppu.bg3.output.above.priority > priority) {
    priority = ppu.bg3.output.above.priority;
    math.above.color = paletteColor(ppu.bg3.output.above.palette);
    math.below.colorEnable = io.bg3.colorEnable;
  }
  if(ppu.bg4.output.above.priority > priority) {
    priority = ppu.bg4.output.above.priority;
    math.above.color = paletteColor(ppu.bg4.output.above.palette);
    math.below.colorEnable = io.bg4.colorEnable;
  }
  if(ppu.obj.output.above.priority > priority) {
    priority = ppu.obj.output.above.priority;
    math.above.color = paletteColor(ppu.obj.output.above.palette);
    math.below.colorEnable = io.obj.colorEnable && ppu.obj.output.above.palette >= 192;
  }
  if(priority == 0) {
    math.above.color = paletteColor(0);
    math.below.colorEnable = io.back.colorEnable;
  }

  if(!ppu.window.output.below.colorEnable) math.below.colorEnable = false;
  math.above.colorEnable = ppu.window.output.above.colorEnable;
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

alwaysinline auto PPU::DAC::paletteColor(n8 palette) const -> n15 {
  ppu.latch.cgramAddress = palette;
  return cgram[palette];
}

alwaysinline auto PPU::DAC::directColor(n8 palette, n3 paletteGroup) const -> n15 {
  //palette = -------- BBGGGRRR
  //group   = -------- -----bgr
  //output  = 0BBb00GG Gg0RRRr0
  return (palette << 7 & 0x6000) + (paletteGroup << 10 & 0x1000)
       + (palette << 4 & 0x0380) + (paletteGroup <<  5 & 0x0040)
       + (palette << 2 & 0x001c) + (paletteGroup <<  1 & 0x0002);
}

alwaysinline auto PPU::DAC::fixedColor() const -> n15 {
  return io.colorRed << 0 | io.colorGreen << 5 | io.colorBlue << 10;
}

auto PPU::DAC::power() -> void {
  random.array({cgram, sizeof(cgram)});
  for(auto& word : cgram) word &= 0x7fff;

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
