auto PPU::Window::scanline(u32 y) -> void {
  if(y == io.y1) v = true;
  if(y == io.y2) v = false;
}

auto PPU::Window::run(u32 x, u32 y) -> void {
  if(x == io.x1) h = true;
  if(x == io.x2) h = false;
  output[x] = h && v;
}

auto PPU::Window::power(u32 id) -> void {
  this->id = id;

  io = {};
  for(auto& flag : output) flag = 0;
  h = 0;
  v = 0;
}
