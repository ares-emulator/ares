auto VDP::scanline() -> void {
  if(state.vcounter < screenHeight()) {
    planeA.scanline(state.vcounter);
    window.scanline(state.vcounter);
    planeB.scanline(state.vcounter);
    sprite.scanline(state.vcounter);
  }

  if(state.vcounter == 240) {
    if(latch.interlace == 0) screen->setProgressive(1);
    if(latch.interlace == 1) screen->setInterlace(latch.field);
    screen->setViewport(0, 0, screen->width(), screen->height());
    screen->frame();
    scheduler.exit(Event::Frame);
  }

  state.output = nullptr;
  if(overscan->value() == 0 && latch.overscan == 0) {
    if(state.vcounter >= 224) return;
    state.output = screen->pixels().data() + (state.vcounter - 0) * 2 * 1280;
  }
  if(overscan->value() == 0 && latch.overscan == 1) {
    if(state.vcounter <=   7) return;
    if(state.vcounter >= 232) return;
    state.output = screen->pixels().data() + (state.vcounter - 8) * 2 * 1280;
  }
  if(overscan->value() == 1 && latch.overscan == 0) {
    if(state.vcounter >= 232) return;
    state.output = screen->pixels().data() + (state.vcounter + 8) * 2 * 1280;
  }
  if(overscan->value() == 1 && latch.overscan == 1) {
    state.output = screen->pixels().data() + (state.vcounter + 0) * 2 * 1280;
  }
  if(latch.interlace) state.output += state.field * 1280;
}

auto VDP::run() -> void {
  if(!io.displayEnable) return outputPixel(0);
  if(state.vcounter >= screenHeight()) return outputPixel(0);

  auto& planeA = window.isWindowed(state.hdot, state.vcounter) ? window : this->planeA;
  planeA.run(state.hdot, state.vcounter);
  planeB.run(state.hdot, state.vcounter);
  sprite.run(state.hdot, state.vcounter);

  Pixel g = {io.backgroundColor, 0};
  Pixel a = planeA.output;
  Pixel b = planeB.output;
  Pixel s = sprite.output;

  auto& bg = a.above() || a.color && !b.above() ? a : b.color ? b : g;
  auto& fg = s.above() || s.color && !b.above() && !a.above() ? s : bg;

  if(!io.shadowHighlightEnable) {
    auto color = cram.read(fg.color);
    outputPixel(1 << 9 | color);
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
    outputPixel(mode << 9 | color);
  }
}

auto VDP::outputPixel(n32 color) -> void {
  if(!state.output) return;
  for(u32 n : range(pixelWidth())) state.output[n] = color;
  state.output += pixelWidth();
}
