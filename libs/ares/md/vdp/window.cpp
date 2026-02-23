auto VDP::Window::begin() -> void {
  latch.hoffset = io.hoffset;
  latch.hdirection = io.hdirection;
  latch.voffset = io.voffset;
  latch.vdirection = io.vdirection;
}

//called 17 (H32) or 21 (H40) times
auto VDP::Window::attributesFetch(s32 attributesIndex) -> void {
  //todo: what should happen for the window on column -1?
  if(attributesIndex == -1) {
    vdp.layerA.windowed[0] = 0;
    vdp.layerA.windowed[1] = 0;
    return;
  }

  s32 x = attributesIndex << 4;
  s32 y = vdp.vcounter();
  vdp.layerA.windowed[0] = vdp.layerA.windowed[1];
  vdp.layerA.windowed[1] = x < latch.hoffset ^ latch.hdirection || y < latch.voffset ^ latch.vdirection;
  if(!vdp.layerA.windowed[1]) return;

  vdp.layerA.attributes.address = vdp.h40() ? nametableAddress & ~0x400 : nametableAddress & ~0;
  vdp.layerA.attributes.hmask   = vdp.h40() ? 63 : 31;
  vdp.layerA.attributes.vmask   = 31;
  vdp.layerA.attributes.hscroll = 0;
  vdp.layerA.attributes.vscroll = 0;
}

auto VDP::Window::power(bool reset) -> void {
  latch = {};
  io = {};
  nametableAddress = 0;
}
