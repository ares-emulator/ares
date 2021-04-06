//returns true if x,y are within the layer A window area
auto VDP::Window::test(u32 x, u32 y) const -> bool {
  if(x < hoffset ^ hdirection) return true;
  if(y < voffset ^ vdirection) return true;
  return false;
}

auto VDP::Window::attributes() const -> Attributes {
  Attributes attributes;
  attributes.address = vdp.h40() ? nametableAddress & ~0x400 : nametableAddress & ~0;
  attributes.width   = vdp.h40() ? 64 : 32;
  attributes.height  = 32;
  return attributes;
}

auto VDP::Window::power(bool reset) -> void {
  hoffset = 0;
  hdirection = 0;
  voffset = 0;
  vdirection = 0;
  nametableAddress = 0;
}
