auto VDP::pixels() -> u32* {
  u32* output = nullptr;
  if(overscan->value() == 0 && latch.overscan == 0) {
    if(state.vcounter >= 224) return nullptr;
    output = screen->pixels().data() + (state.vcounter - 0) * 2 * 1280;
  }
  if(overscan->value() == 0 && latch.overscan == 1) {
    if(state.vcounter <=   7) return nullptr;
    if(state.vcounter >= 232) return nullptr;
    output = screen->pixels().data() + (state.vcounter - 8) * 2 * 1280;
  }
  if(overscan->value() == 1 && latch.overscan == 0) {
    if(state.vcounter >= 232) return nullptr;
    output = screen->pixels().data() + (state.vcounter + 8) * 2 * 1280;
  }
  if(overscan->value() == 1 && latch.overscan == 1) {
    output = screen->pixels().data() + (state.vcounter + 0) * 2 * 1280;
  }
  if(latch.interlace) output += state.field * 1280;
  return output;
}

auto VDP::scanline() -> void {
  if(vcounter() < screenHeight()) {
    sprite.scanline(vcounter());
  }

  if(vcounter() == 240) {
    if(latch.interlace == 0) screen->setProgressive(1);
    if(latch.interlace == 1) screen->setInterlace(state.field);
    screen->setViewport(0, 0, screen->width(), screen->height());
    screen->frame();
    scheduler.exit(Event::Frame);
  }

  state.output = pixels();
}

auto VDP::run() -> void {
  if(!io.displayEnable) return outputPixel(0);
  if(vcounter() >= screenHeight()) return outputPixel(0);

  auto x = hdot();
  auto y = vcounter();
  layers.vscrollFetch(x);
  layerA.run(x, y, window.test(x, y) ? window.attributes() : layerA.attributes());
  layerB.run(x, y, layerB.attributes());
  sprite.run(x, y);

  Pixel g = {io.backgroundColor, 0, 1};
  Pixel a = layerA.output;
  Pixel b = layerB.output;
  Pixel s = sprite.output;

  auto& bg = a.above() || a.color && !b.above() ? a : b.color ? b : g;
  auto& fg = s.above() || s.color && !b.above() && !a.above() ? s : bg;

  if(!io.shadowHighlightEnable) {
    auto color = cram.read(fg.color);
    outputPixel(fg.backdrop << 11 | 1 << 9 | color);
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

    auto color = cram.read(fg.color);
    outputPixel(fg.backdrop << 11 | mode << 9 | color);
  }
}

auto VDP::outputPixel(n32 color) -> void {
  *state.output++ = color;
  *state.output++ = color;
  *state.output++ = color;
  *state.output++ = color;
  if(h40()) return;
  *state.output++ = color;
}
