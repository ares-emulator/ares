auto VDP::pixels() -> u32* {
  u32* output = nullptr;
  if(overscan->value() == 0 && io.overscan == 0) {
    if(state.vcounter >= 224) return nullptr;
    output = screen->pixels().data() + (state.vcounter - 0) * 2 * 1280;
  }
  if(overscan->value() == 0 && io.overscan == 1) {
    if(state.vcounter <=   7) return nullptr;
    if(state.vcounter >= 232) return nullptr;
    output = screen->pixels().data() + (state.vcounter - 8) * 2 * 1280;
  }
  if(overscan->value() == 1 && io.overscan == 0) {
    if(state.vcounter >= 232) return nullptr;
    output = screen->pixels().data() + (state.vcounter + 8) * 2 * 1280;
  }
  if(overscan->value() == 1 && io.overscan == 1) {
    output = screen->pixels().data() + (state.vcounter + 0) * 2 * 1280;
  }
  if(latch.interlace) output += state.field * 1280;
  return output;
}

auto VDP::scanline() -> void {
}

auto VDP::render() -> void {
  if(!io.displayEnable) return;

  if(state.vcounter < window.io.verticalOffset ^ window.io.verticalDirection) {
    window.renderWindow(0, screenWidth());
  } else if(!window.io.horizontalDirection) {
    window.renderWindow(0, window.io.horizontalOffset);
    planeA.renderScreen(window.io.horizontalOffset, screenWidth());
  } else {
    planeA.renderScreen(0, window.io.horizontalOffset);
    window.renderWindow(window.io.horizontalOffset, screenWidth());
  }
  planeB.renderScreen(0, screenWidth());
  sprite.render();

  state.output = pixels();
  if(!state.output) return;

  auto A = &planeA.pixels[0];
  auto B = &planeB.pixels[0];
  auto S = &sprite.pixels[0];
  n7 c[4] = {0, 0, 0, io.backgroundColor};
  if(!io.shadowHighlightEnable) {
    auto p = &cram.palette[1 << 7];
    for(u32 x : range(screenWidth())) {
      c[0] = *A++;
      c[1] = *B++;
      c[2] = *S++;
      u32 l = lookupFG[c[0] >> 2 << 10 | c[1] >> 2 << 5 | c[2] >> 2];
      outputPixel(p[c[l]] | (l == 3) << 11);
    }
  } else {
    auto p = &cram.palette[0 << 7];
    for(u32 x : range(screenWidth())) {
      c[0] = *A++;
      c[1] = *B++;
      c[2] = *S++;
      u32 l = lookupFG[c[0] >> 2 << 10 | c[1] >> 2 << 5 | c[2] >> 2];
      u32 mode = (c[0] | c[1]) >> 2 & 1;  //0 = shadow, 1 = normal, 2 = highlight
      if(l == 2) {
        if(c[2] >= 0x70) {
          if(c[2] <= 0x72) mode = 1;
          else if(c[2] == 0x73) l = lookupBG[c[0] >> 2 << 5 | c[1] >> 2], mode++;
          else if(c[2] == 0x7b) l = lookupBG[c[0] >> 2 << 5 | c[1] >> 2], mode = 0;
          else mode |= c[2] >> 2 & 1;
        } else {
          mode |= c[2] >> 2 & 1;
        }
      }
      outputPixel(p[mode << 7 | c[l]] | (l == 3) << 11);
    }
  }
}

auto VDP::outputPixel(n32 color) -> void {
  for(u32 n : range(pixelWidth())) state.output[n] = color;
  state.output += pixelWidth();
}
