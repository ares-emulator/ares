auto VDP::DAC::begin() -> void {
  pixels = vdp.pixels();
}

auto VDP::DAC::pixel() -> void {
  if(!vdp.io.displayEnable) return output(0);
  if(vdp.vcounter() >= vdp.screenHeight()) return output(0);

  Pixel g = {vdp.io.backgroundColor, 0, 1};
  Pixel a = vdp.layerA.pixel();
  Pixel b = vdp.layerB.pixel();
  Pixel s = vdp.sprite.pixel();

  auto& bg = a.above() || a.color && !b.above() ? a : b.color ? b : g;
  auto& fg = s.above() || s.color && !b.above() && !a.above() ? s : bg;

  if(!vdp.io.shadowHighlightEnable) {
    auto color = vdp.cram.color(fg.color);
    output(fg.backdrop << 11 | 1 << 9 | color);
  } else {
    u32 mode = a.priority || b.priority;  //0 = shadow, 1 = normal, 2 = highlight

    if(&fg == &s) switch(s.color) {
    case 0x0e:
    case 0x1e:
    case 0x2e: mode  = 1; break;
    case 0x3e: mode += 1; fg = bg; break;
    case 0x3f: mode  = 0; fg = bg; break;
    default:   mode |= s.priority; break;
    }

    auto color = vdp.cram.color(fg.color);
    output(fg.backdrop << 1 | mode << 9 | color);
  }
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
