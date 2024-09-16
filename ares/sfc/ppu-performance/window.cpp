auto PPU::Window::render(Layer& layer, bool enable, bool output[448]) -> void {
  if(!enable || (!layer.oneEnable && !layer.twoEnable)) {
    memory::fill<bool>(output, 256, 0);
    return;
  }

  s32 oneLeft  = io.oneLeft;
  s32 oneRight = io.oneRight;
  s32 twoLeft  = io.twoLeft;
  s32 twoRight = io.twoRight;

  if(layer.oneEnable && !layer.twoEnable) {
    bool set = 1 ^ layer.oneInvert, clear = !set;
    for(s32 x : range(256)) {
      output[x] = x >= oneLeft && x <= oneRight ? set : clear;
    }
    return;
  }

  if(layer.twoEnable && !layer.oneEnable) {
    bool set = 1 ^ layer.twoInvert, clear = !set;
    for(s32 x : range(256)) {
      output[x] = x >= twoLeft && x <= twoRight ? set : clear;
    }
    return;
  }

  for(s32 x : range(256)) {
    bool oneMask = (x >= oneLeft && x <= oneRight) ^ layer.oneInvert;
    bool twoMask = (x >= twoLeft && x <= twoRight) ^ layer.twoInvert;
    switch(layer.mask) {
    case 0: output[x] = (oneMask | twoMask) == 1; break;
    case 1: output[x] = (oneMask & twoMask) == 1; break;
    case 2: output[x] = (oneMask ^ twoMask) == 1; break;
    case 3: output[x] = (oneMask ^ twoMask) == 0; break;
    }
  }
}

auto PPU::Window::render(Color& color, u32 mask, bool output[448]) -> void {
  bool set = 0;
  bool clear = 0;
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

  s32 oneLeft  = io.oneLeft;
  s32 oneRight = io.oneRight;
  s32 twoLeft  = io.twoLeft;
  s32 twoRight = io.twoRight;

  if(color.oneEnable && !color.twoEnable) {
    if(color.oneInvert) set ^= 1, clear ^= 1;
    for(s32 x : range(256)) {
      output[x] = x >= oneLeft && x <= oneRight ? set : clear;
    }
    return;
  }

  if(!color.oneEnable && color.twoEnable) {
    if(color.twoInvert) set ^= 1, clear ^= 1;
    for(s32 x : range(256)) {
      output[x] = x >= twoLeft && x <= twoRight ? set : clear;
    }
    return;
  }

  for(s32 x : range(256)) {
    bool oneMask = (x >= oneLeft && x <= oneRight) ^ color.oneInvert;
    bool twoMask = (x >= twoLeft && x <= twoRight) ^ color.twoInvert;
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
