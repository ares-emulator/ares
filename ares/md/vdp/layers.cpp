auto VDP::Layers::hscrollFetch() -> void {
  static const u32 mask[] = {0u, 7u, ~7u, ~0u};
  n16 address = hscrollAddress;
  address += (vdp.vcounter() & mask[hscrollMode]) << 1;
  vdp.layerA.hscroll = vdp.vram.read(address | 0);
  vdp.layerB.hscroll = vdp.vram.read(address | 1);
}

auto VDP::Layers::vscrollFetch(u32 x) -> void {
  n16 address = (x >> 4 & 0 - vscrollMode) << 1;
  vdp.layerA.vscroll = vdp.vsram.read(address | 0);
  vdp.layerB.vscroll = vdp.vsram.read(address | 1);
}

auto VDP::Layers::power(bool reset) -> void {
  hscrollMode = 0;
  hscrollAddress = 0;
  vscrollMode = 0;
  nametableWidth = 0;
  nametableHeight = 0;
}
