auto VDP::Layer::begin() -> void {
  for(auto& pixel : pixels) pixel = {};
}

//called 17 (H32) or 21 (H40) times
auto VDP::Layer::attributesFetch() -> void {
  attributes.address = nametableAddress;
  attributes.hmask   = 32 * (1 + vdp.layers.nametableWidth ) - 1;
  attributes.vmask   = 32 * (1 + vdp.layers.nametableHeight) - 1;
  attributes.hscroll = hscroll;
  attributes.vscroll = vscroll;

  //prohibited vsize=2 acts like vsize=3 but ignores a6
  if(vdp.layers.nametableHeight == 2) {
    attributes.vmask = 32 * (1 + 3) - 1 & ~(1 << 6);
  }

  //prohibited hsize=2 causes first row to repeat (overrides all vsize modes)
  if(vdp.layers.nametableWidth == 2) {
    attributes.hmask = 31;  //todo: this is not 100% accurate (but is close)
    attributes.vmask =  0;
  }
}

//called 17 (H32) ot 21 (H40) times
auto VDP::Layer::mappingFetch(s32 mappingIndex) -> void {
  if(!vdp.displayEnable()) {
    return vdp.slot();
  }

  auto interlace = vdp.io.interlaceMode == 3;
  auto x = mappingIndex * 16;
  auto y = vdp.vcounter();
  if(interlace) y = y << 1 | vdp.field();

  x -= attributes.hscroll & ~15;
  y += attributes.vscroll;

  auto tileX = x >> 3 & attributes.hmask;
  auto tileY = y >> 3 + interlace & attributes.vmask;
  n15 address = attributes.address + n12(tileY * (1 + attributes.hmask) + tileX);
  for(auto& mapping : mappings) {
    auto data = vdp.vram.read(address++);
    mapping.address  = data.bit(0,10) << 4 + interlace;
    mapping.hflip    = data.bit(11);
    mapping.palette  = data.bit(13,14);
    mapping.priority = data.bit(15);

    auto pixelY = y & 7 + interlace * 8;
    if(data.bit(12)) pixelY ^= 7 + interlace * 8;
    mapping.address += pixelY << 1;

    u32 extra = mapping.priority << 2 | mapping.palette;
    extra |= extra  <<  4;
    extra |= extra  <<  8;
    extra |= extra  << 16;
    extras = extras << 32 | extra;
  }
}

//called 34 (H32) or 42 (H40) times
auto VDP::Layer::patternFetch(u32 patternIndex) -> void {
  if(!vdp.displayEnable()) {
    return vdp.slot();
  }

  auto interlace = vdp.io.interlaceMode == 3;
  auto y = vdp.vcounter() + attributes.vscroll;
  if(interlace) y = y << 1 | vdp.field();

  auto& mapping = mappings[patternIndex & 1];
  n15 address = mapping.address;
  u16 hi = vdp.vram.read(generatorAddress | address++);
  u16 lo = vdp.vram.read(generatorAddress | address++);
  u32 data = hi << 16 | lo << 0;
  if(mapping.hflip) data = hflip(data);
  colors = colors << 32 | data;

  if(patternIndex & 1) {
    u32 pixelCount = (patternIndex >> 1) << 4;
    for(auto index : reverse(range(16))) {
      n8 shift = (index + (attributes.hscroll & 15)) * 4;
      if(windowed[0] && !windowed[1] && hscroll & 15) {
        //emulate left windowing bug when fine hscroll != 0 (repeats next column)
        if(15 - index < (hscroll & 15)) shift += 64;
      }
      n6 color = colors >> shift & 15;
      n4 extra = extras >> shift & 15;
      if(color) color |= extra.bit(0,1) << 4;
      pixels[pixelCount++] = {color, extra.bit(2)};
    }
  }
}

auto VDP::Layer::pixel(u32 pixelIndex) -> Pixel {
  return pixels[16 + pixelIndex];
}

auto VDP::Layer::power(bool reset) -> void {
  hscroll = 0;
  vscroll = 0;
  generatorAddress = 0;
  nametableAddress = 0;
  attributes = {};
  for(auto& pixel : pixels) pixel = {};
  for(auto& mapping : mappings) mapping = {};
}
