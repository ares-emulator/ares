auto VDP::Layers::hscrollFetch() -> void {
  if(!vdp.displayEnable()) {
    return vdp.slot();
  }

  static const u32 mask[] = {0u, 7u, ~7u, ~0u};
  n16 address = hscrollAddress;
  address += ((u8)vdp.vcounter() & mask[hscrollMode]) << 1;
  vdp.layerA.hscroll = vdp.vram.read(address++);
  vdp.layerB.hscroll = vdp.vram.read(address++);
}

auto VDP::Layers::vscrollFetch() -> void {
  if(vscrollMode == 1) return;
  vdp.layerA.vscroll = vdp.vsram.read(0);
  vdp.layerB.vscroll = vdp.vsram.read(1);
}

auto VDP::Layers::vscrollFetch(s32 index) -> void {
  if(vscrollMode == 0) return;
  n16 address = index << 1;
  vdp.layerA.vscroll = vdp.vsram.read(address++);
  vdp.layerB.vscroll = vdp.vsram.read(address++);
}

auto VDP::Layers::power(bool reset) -> void {
  hscrollMode = 0;
  hscrollAddress = 0;
  vscrollMode = 0;
  nametableWidth = 0;
  nametableHeight = 0;
}
