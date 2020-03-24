auto PPU::Window::render(Layer& layer, bool enable, bool output[256]) -> void {
  if(!enable || (!layer.oneEnable && !layer.twoEnable)) {
    memory::fill<bool>(output, 256, 0);
    return;
  }

  if(layer.oneEnable && !layer.twoEnable) {
    bool set = 1 ^ layer.oneInvert, clear = !set;
    for(uint x : range(256)) {
      output[x] = x >= io.oneLeft && x <= io.oneRight ? set : clear;
    }
    return;
  }

  if(layer.twoEnable && !layer.oneEnable) {
    bool set = 1 ^ layer.twoInvert, clear = !set;
    for(uint x : range(256)) {
      output[x] = x >= io.twoLeft && x <= io.twoRight ? set : clear;
    }
    return;
  }

  for(uint x : range(256)) {
    bool oneMask = (x >= io.oneLeft && x <= io.oneRight) ^ layer.oneInvert;
    bool twoMask = (x >= io.twoLeft && x <= io.twoRight) ^ layer.twoInvert;
    switch(layer.mask) {
    case 0: output[x] = (oneMask | twoMask) == 1; break;
    case 1: output[x] = (oneMask & twoMask) == 1; break;
    case 2: output[x] = (oneMask ^ twoMask) == 1; break;
    case 3: output[x] = (oneMask ^ twoMask) == 0; break;
    }
  }
}

auto PPU::Window::render(Color& color, uint mask, bool output[256]) -> void {
  bool set, clear;
  switch(mask) {
  case 0: memory::fill<bool>(output, 256, 1); return;  //always
  case 1: set = 1, clear = 0; break;  //inside
  case 2: set = 0, clear = 1; break;  //outside
  case 3: memory::fill<bool>(output, 256, 0); return;  //never
  }

  if(!color.oneEnable && !color.twoEnable) {
    memory::fill<bool>(output, 256, clear);
    return;
  }

  if(color.oneEnable && !color.twoEnable) {
    if(color.oneInvert) set ^= 1, clear ^= 1;
    for(uint x : range(256)) {
      output[x] = x >= io.oneLeft && x <= io.oneRight ? set : clear;
    }
    return;
  }

  if(!color.oneEnable && color.twoEnable) {
    if(color.twoInvert) set ^= 1, clear ^= 1;
    for(uint x : range(256)) {
      output[x] = x >= io.twoLeft && x <= io.twoRight ? set : clear;
    }
    return;
  }

  for(uint x : range(256)) {
    bool oneMask = (x >= io.oneLeft && x <= io.oneRight) ^ color.oneInvert;
    bool twoMask = (x >= io.twoLeft && x <= io.twoRight) ^ color.twoInvert;
    switch(color.mask) {
    case 0: output[x] = (oneMask | twoMask) == 1 ? set : clear; break;
    case 1: output[x] = (oneMask & twoMask) == 1 ? set : clear; break;
    case 2: output[x] = (oneMask ^ twoMask) == 1 ? set : clear; break;
    case 3: output[x] = (oneMask ^ twoMask) == 0 ? set : clear; break;
    }
  }
}

auto PPU::Window::power() -> void {
  io = {};
}
