auto VDP::DAC::pixel(u32 x) -> void {
  if(!pixels) return;

  Pixel g = {vdp.io.backgroundColor, 0, 1};
  Pixel a = vdp.layerA.pixel(x);
  Pixel b = vdp.layerB.pixel(x);
  Pixel s = vdp.sprite.pixel(x);

  if(test.disableLayers == 1) {
    if(test.forceLayer == 1) g = s;
    if(test.forceLayer == 2) g = a;
    if(test.forceLayer == 3) g = b;
    a = {};
    b = {};
    s = {};
  }

  auto& bg = a.above() || a.color && !b.above() ? a : b.color ? b : g;
  auto& fg = s.above() || s.color && !b.above() && !a.above() ? s : bg;

  auto pixel = fg;
  auto mode  = 1;  //0 = shadow, 1 = normal, 2 = highlight

  if(vdp.io.shadowHighlightEnable) {
    mode = a.priority || b.priority;
    if(&fg == &s) switch(s.color) {
    case 0x0e:
    case 0x1e:
    case 0x2e: mode  = 1; break;
    case 0x3e: mode += 1; pixel = bg; break;
    case 0x3f: mode  = 0; pixel = bg; break;
    default:   mode |= s.priority; break;
    }
  }

  if(test.disableLayers == 0) {
    if(test.forceLayer == 1) {
      if(pixel.backdrop) pixel = s;
      pixel.color &= s.color;
    }
    if(test.forceLayer == 2) {
      if(pixel.backdrop) pixel = a;
      pixel.color &= a.color;
    }
    if(test.forceLayer == 3) {
      if(pixel.backdrop) pixel = b;
      pixel.color &= b.color;
    }
  }

  auto color = vdp.cram.color(pixel.color);
  output(pixel.backdrop << 11 | mode << 9 | color);
}

auto VDP::DAC::output(n32 color) -> void {
  *pixels++ = color;
  *pixels++ = color;
  *pixels++ = color;
  *pixels++ = color;
  if(vdp.h40()) return;
  *pixels++ = color;
}

auto VDP::DAC::power(bool reset) -> void {
  test = {};
  pixels = nullptr;
}
