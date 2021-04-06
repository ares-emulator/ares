auto VDP::Layer::mappingFetch() -> void {
}

auto VDP::Layer::patternFetch() -> void {
}

auto VDP::Layer::attributes() const -> Attributes {
  Attributes attributes;
  attributes.address = nametableAddress;
  attributes.width   = 32 * (1 + vdp.layers.nametableWidth);
  attributes.height  = 32 * (1 + vdp.layers.nametableHeight);
  attributes.hscroll = hscroll;
  attributes.vscroll = vscroll;
  return attributes;
}

auto VDP::Layer::run(u32 x, u32 y, const Attributes& attributes) -> void {
  bool interlace = vdp.io.interlaceMode == 3;
  if(interlace) y = y << 1 | vdp.field();

  x -= attributes.hscroll;
  y += attributes.vscroll;

  auto tileX = x >> 3 & attributes.width - 1;
  auto tileY = y >> 3 + interlace & attributes.height - 1;
  auto address = attributes.address;
  address += n12(tileY * attributes.width + tileX);

  n16 tileAttributes = vdp.vram.read(address);
  n15 tileAddress = tileAttributes.bit(0,10) << 4 + interlace;

  auto pixelX = x & 7;
  auto pixelY = y & 7 + interlace * 8;
  if(tileAttributes.bit(11)) pixelX ^= 7;
  if(tileAttributes.bit(12)) pixelY ^= 7 + interlace * 8;
  tileAddress += pixelY << 1 | pixelX >> 2;

  n16 tileData = vdp.vram.read(generatorAddress | tileAddress);
  n4  color = tileData >> ((pixelX & 3 ^ 3) << 2);
  output.color = color ? tileAttributes.bit(13,14) << 4 | color : 0;
  output.priority = tileAttributes.bit(15);
}

auto VDP::Layer::power(bool reset) -> void {
  hscroll = 0;
  vscroll = 0;
  generatorAddress = 0;
  nametableAddress = 0;
  output = {};
}
