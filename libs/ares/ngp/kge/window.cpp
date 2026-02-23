auto KGE::Window::render(n8 x, n8 y) -> maybe<Output&> {
  if(x >= hoffset && y >= voffset && x < hoffset + hlength && y < voffset + vlength) {
    return {};
  }

  output.color = color;
  return output;
}

auto KGE::Window::power() -> void {
  output  = {};
  hoffset = 0;
  voffset = 0;
  hlength = 0;
  vlength = 0;
  color   = 0;
}
