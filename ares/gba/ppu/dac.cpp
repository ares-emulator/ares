auto PPU::DAC::scanline(u32 y) -> void {
  line = ppu.screen->pixels().data() + y * 240;
}

auto PPU::DAC::upperLayer(u32 x, u32 y) -> void {
  if(ppu.blank()) {
    color = 0x7fff;
    blending = false;
    return;
  }

  //determine active window
  n1 active[6] = {true, true, true, true, true, true};  //enable all layers if no windows are enabled
  if(ppu.window0.io.enable || ppu.window1.io.enable || ppu.window2.io.enable) {
    memory::copy(&active, &ppu.window3.io.active, sizeof(active));
    if(ppu.window2.io.enable && ppu.window2.output) memory::copy(&active, &ppu.window2.io.active, sizeof(active));
    if(ppu.window1.io.enable && ppu.window1.output) memory::copy(&active, &ppu.window1.io.active, sizeof(active));
    if(ppu.window0.io.enable && ppu.window0.output) memory::copy(&active, &ppu.window0.io.active, sizeof(active));
  }

  //get background and object pixels
  ppu.objects.outputPixel(x, y);
  ppu.bg0.outputPixel(x, y);
  ppu.bg1.outputPixel(x, y);
  ppu.bg2.outputPixel(x, y);
  ppu.bg3.outputPixel(x, y);

  //priority sorting: find topmost two pixels
  layers[OBJ] = ppu.objects.mosaic;
  layers[BG0] = ppu.bg0.mosaic;
  layers[BG1] = ppu.bg1.mosaic;
  layers[BG2] = ppu.bg2.mosaic;
  layers[BG3] = ppu.bg3.mosaic;
  layers[SFX] = {true, 3, 0};

  aboveLayer = 5;
  belowLayer = 5;
  for(s32 priority = 3; priority >= 0; priority--) {
    for(s32 layer = 5; layer >= 0; layer--) {
      if(layers[layer].enable && layers[layer].priority == priority && active[layer]) {
        belowLayer = aboveLayer;
        aboveLayer = layer;
      }
    }
  }

  auto above = layers[aboveLayer];
  color = pramLookup(above);

  //color blending
  if(above.translucent && io.blendBelow[belowLayer]) {
    blending = true;
    return;
  }
  if(active[SFX]) {
    auto evy = min(16u, (u32)io.blendEVY);
    if(io.blendMode == 1 && io.blendAbove[aboveLayer] && io.blendBelow[belowLayer]) {
      blending = true;
      return;
    } else if(io.blendMode == 2 && io.blendAbove[aboveLayer]) {
      color = blend(color, 16 - evy, 0x7fff, evy);
    } else if(io.blendMode == 3 && io.blendAbove[aboveLayer]) {
      color = blend(color, 16 - evy, 0x0000, evy);
    }
  }

  blending = false;
}

auto PPU::DAC::lowerLayer(u32 x, u32 y) -> void {
  if(blending) {
    auto below = layers[belowLayer];
    auto eva = min(16u, (u32)io.blendEVA);
    auto evb = min(16u, (u32)io.blendEVB);

    color = blend(color, eva, pramLookup(below), evb);
  }

  line[x] = color;
}

inline auto PPU::DAC::pramLookup(Pixel& layer) -> n15 {
  if(layer.directColor) return layer.color;
  return ppu.pram[layer.color];
}

auto PPU::DAC::blend(n15 above, u32 eva, n15 below, u32 evb) -> n15 {
  n5 ar = above >> 0, ag = above >> 5, ab = above >> 10;
  n5 br = below >> 0, bg = below >> 5, bb = below >> 10;

  u32 r = (ar * eva + br * evb) >> 4;
  u32 g = (ag * eva + bg * evb) >> 4;
  u32 b = (ab * eva + bb * evb) >> 4;

  return min(31u, r) << 0 | min(31u, g) << 5 | min(31u, b) << 10;
}

auto PPU::DAC::power() -> void {
  io = {};
}
