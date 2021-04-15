auto VDP::Layers::begin() -> void {
  vscrollIndex = -1;
}

auto VDP::Layers::hscrollFetch() -> void {
  static const u32 mask[] = {0u, 7u, ~7u, ~0u};
  n16 address = hscrollAddress;
  address += (vdp.vcounter() & mask[hscrollMode]) << 1;
  vdp.layerA.hscroll = vdp.vram.read(address++);
  vdp.layerB.hscroll = vdp.vram.read(address++);
}

auto VDP::Layers::vscrollFetch() -> void {
  n16 address = (vscrollIndex++ & 0 - vscrollMode) << 1;
  vdp.layerA.vscroll = vdp.vsram.read(address++);
  vdp.layerB.vscroll = vdp.vsram.read(address++);
}

auto VDP::Layers::power(bool reset) -> void {
  hscrollMode = 0;
  hscrollAddress = 0;
  vscrollMode = 0;
  nametableWidth = 0;
  nametableHeight = 0;
  vscrollIndex = 0;
}
