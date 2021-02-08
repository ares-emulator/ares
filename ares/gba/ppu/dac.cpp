auto PPU::DAC::run(u32 x, u32 y) -> n15 {
  if(ppu.blank()) return 0x7fff;

  //determine active window
  n1 active[6] = {true, true, true, true, true, true};  //enable all layers if no windows are enabled
  if(ppu.window0.io.enable || ppu.window1.io.enable || ppu.window2.io.enable) {
    memory::copy(&active, &ppu.window3.io.active, sizeof(active));
    if(ppu.window2.io.enable && ppu.window2.output) memory::copy(&active, &ppu.window2.io.active, sizeof(active));
    if(ppu.window1.io.enable && ppu.window1.output) memory::copy(&active, &ppu.window1.io.active, sizeof(active));
    if(ppu.window0.io.enable && ppu.window0.output) memory::copy(&active, &ppu.window0.io.active, sizeof(active));
  }

  //priority sorting: find topmost two pixels
  Pixel layers[6] = {
    ppu.objects.mosaic,
    ppu.bg0.mosaic,
    ppu.bg1.mosaic,
    ppu.bg2.mosaic,
    ppu.bg3.mosaic,
    {true, 3, ppu.pram[0]},
  };

  u32 aboveLayer = 5, belowLayer = 5;
  for(s32 priority = 3; priority >= 0; priority--) {
    for(s32 layer = 5; layer >= 0; layer--) {
      if(layers[layer].enable && layers[layer].priority == priority && active[layer]) {
        belowLayer = aboveLayer;
        aboveLayer = layer;
      }
    }
  }

  auto above = layers[aboveLayer];
  auto below = layers[belowLayer];
  auto eva = min(16u, (u32)io.blendEVA);
  auto evb = min(16u, (u32)io.blendEVB);
  auto evy = min(16u, (u32)io.blendEVY);
  n15 color = above.color;

  //color blending
  if(active[SFX]) {
    if(above.translucent && io.blendBelow[belowLayer]) {
      color = blend(above.color, eva, below.color, evb);
    } else if(io.blendMode == 1 && io.blendAbove[aboveLayer] && io.blendBelow[belowLayer]) {
      color = blend(above.color, eva, below.color, evb);
    } else if(io.blendMode == 2 && io.blendAbove[aboveLayer]) {
      color = blend(above.color, 16 - evy, 0x7fff, evy);
    } else if(io.blendMode == 3 && io.blendAbove[aboveLayer]) {
      color = blend(above.color, 16 - evy, 0x0000, evy);
    }
  }

  return color;
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
