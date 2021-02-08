auto VPU::renderWindow(n8 x, n8 y) -> bool {
  if(x >= window.hoffset
  && y >= window.voffset
  && x <  window.hoffset + window.hlength
  && y <  window.voffset + window.vlength) return false;

  if(Model::NeoGeoPocket()) {
    window.output = window.color;
  } else {
    window.output = colors[0xf8 + window.color];
  }
  return true;
}
