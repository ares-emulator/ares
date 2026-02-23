template<bool _h40, bool draw> auto VDP::DAC::pixel(u32 x) -> void {
  if(!draw) {
    output<_h40>(0b101 << 9 | vdp.cram.color(vdp.io.backgroundColor));
    return;
  }

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

  auto& bg = a.above() || a.solid() && !b.above() ? a : b.solid() ? b : g;
  auto& fg = s.above() || s.solid() && !b.above() && !a.above() ? s : bg;

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
  output<_h40>(pixel.backdrop << 11 | mode << 9 | color);
}

auto pixelIndex(n9 hpos) -> maybe<u32> {
  if(vdp.h40()) {
    if(hpos < 0x00d || hpos > 0x167) return nothing;
    return (hpos-0x00d)*4;
  } else {
    if(hpos < 0x00b || hpos > 0x125) return nothing;
    return (hpos-0x00b)*5;
  }
}

template<u8 _size, u16 _h32Pos, u16 _h40Pos> inline auto VDP::DAC::fillBorder(n8 ofst) -> void {
  if(!pixels) return;
  if(ofst >= _size) return;

  u32 hpos = (vdp.h40() ? _h40Pos : _h32Pos) + ofst;
  u32 idx = pixelIndex(hpos)();
  n32 px = 0b101 << 9 | vdp.cram.color(vdp.io.backgroundColor);
  for(auto n : range((_size-ofst)*(vdp.h40()?4:5)))
    pixels[idx+n] = px;
}

auto VDP::DAC::fillLeftBorder(n8 ofst) -> void {
  fillBorder<13,0x00b,0x00d>(ofst);
}

auto VDP::DAC::fillRightBorder(n8 ofst) -> void {
  fillBorder<14,0x118,0x15a>(ofst);
}

auto VDP::DAC::dot(n9 hpos, n9 color) -> void {
  if(!pixels) return;

  if(auto i = pixelIndex(hpos)) {
    u32 index = i();
    n32 px = 0b101 << 9 | color;
    pixels[index++] = px;
    pixels[index++] = px;
    pixels[index++] = px;
    pixels[index++] = px;
    if(vdp.h40()) return;
    pixels[index++] = px;
  }
}

template<bool _h40> auto VDP::DAC::output(n32 color) -> void {
  *active++ = color;
  *active++ = color;
  *active++ = color;
  *active++ = color;
  if(_h40) return;
  *active++ = color;
}

auto VDP::DAC::power(bool reset) -> void {
  test = {};
  pixels = nullptr;
  active = nullptr;
}
