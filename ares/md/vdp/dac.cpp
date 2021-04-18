auto VDP::DAC::begin() -> void {
  pixels = vdp.pixels();
}

auto VDP::DAC::pixel() -> void {
  if(!vdp.io.displayEnable) return output(0);
  if(vdp.vcounter() >= vdp.screenHeight()) return output(0);

  Pixel g = {vdp.io.backgroundColor, 0};
  Pixel a = vdp.layerA.pixel();
  Pixel b = vdp.layerB.pixel();
  Pixel s = vdp.sprite.pixel();

  if(vdp.io.debugDisableLayers == 1) {
    if(vdp.io.debugForceLayer == 1) g = s;
    if(vdp.io.debugForceLayer == 2) g = a;
    if(vdp.io.debugForceLayer == 3) g = b;
    a = {};
    b = {};
    s = {};
  }

  auto& bg = a.above() || a.color && !b.above() ? a : b.color ? b : g;
  auto& fg = s.above() || s.color && !b.above() && !a.above() ? s : bg;

  auto color = fg.color;
  auto mode  = 1;  //0 = shadow, 1 = normal, 2 = highlight

  if(vdp.io.shadowHighlightEnable) {
    mode = a.priority || b.priority;
    if(&fg == &s) switch(s.color) {
    case 0x0e:
    case 0x1e:
    case 0x2e: mode  = 1; break;
    case 0x3e: mode += 1; color = bg.color; break;
    case 0x3f: mode  = 0; color = bg.color; break;
    default:   mode |= s.priority; break;
    }
  }

  if(vdp.io.debugDisableLayers == 0) {
    if(vdp.io.debugForceLayer == 1) color &= s.color;
    if(vdp.io.debugForceLayer == 2) color &= a.color;
    if(vdp.io.debugForceLayer == 3) color &= b.color;
  }

  bool backdrop = &fg == &g;
  output(backdrop << 11 | mode << 9 | vdp.cram.color(color));
}

auto VDP::DAC::output(n32 color) -> void {
  if(!pixels) return;
  *pixels++ = color;
  *pixels++ = color;
  *pixels++ = color;
  *pixels++ = color;
  if(vdp.h40()) return;
  *pixels++ = color;
}

auto VDP::DAC::power(bool reset) -> void {
}
