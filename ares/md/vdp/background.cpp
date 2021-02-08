auto VDP::Background::isWindowed(u32 x, u32 y) -> bool {
  if((x < io.horizontalOffset) ^ io.horizontalDirection) return true;
  if((y < io.verticalOffset  ) ^ io.verticalDirection  ) return true;
  return false;
}

auto VDP::Background::updateHorizontalScroll(u32 y) -> void {
  if(id == ID::Window) return;

  n15 address = io.horizontalScrollAddress;

  static const u32 mask[] = {0u, 7u, ~7u, ~0u};
  address += (y & mask[io.horizontalScrollMode]) << 1;
  address += id == ID::PlaneB;

  state.horizontalScroll = vdp.vram.read(address).bit(0,9);
}

auto VDP::Background::updateVerticalScroll(u32 x) -> void {
  if(id == ID::Window) return;

  auto address = (x >> 4 & 0 - io.verticalScrollMode) << 1;
  address += id == ID::PlaneB;

  state.verticalScroll = vdp.vsram.read(address);
}

auto VDP::Background::nametableAddress() -> n15 {
  if(id == ID::Window && vdp.screenWidth() == 320) return io.nametableAddress & ~0x0400;
  return io.nametableAddress;
}

auto VDP::Background::nametableWidth() -> u32 {
  if(id == ID::Window) return vdp.screenWidth() == 320 ? 64 : 32;
  return 32 * (1 + io.nametableWidth);
}

auto VDP::Background::nametableHeight() -> u32 {
  if(id == ID::Window) return 32;
  return 32 * (1 + io.nametableHeight);
}

auto VDP::Background::scanline(u32 y) -> void {
  updateHorizontalScroll(y);
}

auto VDP::Background::run(u32 x, u32 y) -> void {
  updateVerticalScroll(x);

  bool interlace = vdp.io.interlaceMode == 3;
  if(interlace) y = y << 1 | vdp.state.field;

  x -= state.horizontalScroll;
  y += state.verticalScroll;

  u32 tileX = x >> 3 & nametableWidth() - 1;
  u32 tileY = y >> 3 + interlace & nametableHeight() - 1;

  auto address = nametableAddress();
  address += (tileY * nametableWidth() + tileX) & 0x0fff;

  u32 pixelX = x & 7;
  u32 pixelY = y & 7 + interlace * 8;

  n16 tileAttributes = vdp.vram.read(address);
  n15 tileAddress = tileAttributes.bit(0,10) << 4 + interlace;
  if(tileAttributes.bit(11)) pixelX ^= 7;
  if(tileAttributes.bit(12)) pixelY ^= 7 + interlace * 8;
  tileAddress += pixelY << 1 | pixelX >> 2;

  n16 tileData = vdp.vram.read(io.generatorAddress | tileAddress);
  n4  color = tileData >> (((pixelX & 3) ^ 3) << 2);
  output.color = color ? tileAttributes.bit(13,14) << 4 | color : 0;
  output.priority = tileAttributes.bit(15);
}

auto VDP::Background::power() -> void {
  io = {};
  state = {};
}
