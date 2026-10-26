alwaysinline auto GPU::Scanout::readRGB24(u32 sourceX) -> u32 {
  auto* row = self.vram.data + sourceBase;
  if(likely(sourceX <= 2044)) return nall::memory::readl<4, u32>(row + sourceX) & 0x00ff'ffff;
  return row[sourceX] | row[(sourceX + 1) & 2047] << 8 | row[(sourceX + 2) & 2047] << 16;
}

auto GPU::Scanout::queue() -> void {
  self.refreshed = true;

  self.renderer.synchronize();

  if(self.io.interlace)
    self.screen->refreshRateHint(system.gpuFrequency() * self.htotalDenominator() * 2, self.htotalNumerator(), self.io.videoMode ? 625 : 525);
  else
    self.screen->refreshRateHint(system.gpuFrequency() * self.htotalDenominator(), self.htotalNumerator(), self.vtotal());

  dotclock = self.display.dotclock;
  rangeY1 = self.io.displayRangeY1;
  rangeY2 = self.io.displayRangeY2;
  interlace = self.io.interlace;
  interleaved = self.io.verticalResolution && self.io.interlace;
  pal = self.io.videoMode;

  bool field = self.io.field;

  u32 activeX1 = 488;
  u32 activeY1 = pal ? 20 : 16;
  u32 activeX2 = pal ? 3300 : 3288;
  u32 activeY2 = pal ? 308 : 256;
  u32 viewX1 = self.screen->overscan() ? activeX1 : 608;
  u32 viewX2 = self.screen->overscan() ? activeX2 : 3168;
  u32 viewY1 = self.screen->overscan() ? activeY1 : (pal ? 31 : 24);
  u32 viewY2 = self.screen->overscan() ? activeY2 : (pal ? 295 : 248);
  u32 activeWidth = (activeX2 - activeX1 + ClockScale - 1) / ClockScale;
  u32 activeHeight = (activeY2 - activeY1) * LineScale;
  u32 viewWidth = (viewX2 - viewX1 + ClockScale - 1) / ClockScale;
  u32 viewHeight = (viewY2 - viewY1) * LineScale;

  if(interleaved) self.screen->setInterlace(field);
  else self.screen->setProgressive(1);
  self.screen->setScale(1.0, 1.0);
  self.screen->setAspect(4.0 * activeHeight, 3.0 * activeWidth);
  self.screen->setSize(viewWidth, viewHeight);
  self.screen->setViewport((viewX1 - activeX1) / ClockScale, (viewY1 - activeY1) * LineScale, viewWidth, viewHeight);
  self.screen->frame();
  scheduler.exit(Event::Frame);
}

auto GPU::Scanout::lineStart() -> void {
  accurate = self.pixelAccuracySetting->value();
  u32 signalY = self.io.vcounter;
  u32 activeY1 = pal ? 20 : 16;
  u32 activeY2 = pal ? 308 : 256;

  lineVisible = signalY < 512 && signalY >= rangeY1 && signalY < rangeY2 && signalY >= activeY1 && signalY < activeY2;

  if(!lineVisible) return;

  depth = self.io.colorDepth;
  u32 row = signalY - rangeY1;

  bool displayField = self.io.field;
  targetY = (signalY - activeY1) * LineScale + (interleaved ? (u32)displayField : 0);

  bool sourceField = self.io.displayField;
  u32 lineStartY = self.io.displayStartY;
  u32 sourceY = (lineStartY + (interleaved ? row * 2 + sourceField : row)) & 511;
  sourceBase = sourceY * 2048;
  sourceStartX = self.io.displayStartX;

  outputPtr = self.screen->pixels(0).data() + targetY * OutputPitch;
  srcPixelX = 0;
  hasPrevPixel = false;

  u32 rangeX1 = self.io.displayRangeX1;
  u32 rangeX2 = self.io.displayRangeX2;
  displayCountLatched = false;
  displayOutputActive = false;
  displayPixelsRemaining = 0;

  if(rangeX1 == 0 && rangeX2 > rangeX1) {
    displayCountLatched = true;
    displayPixelsRemaining = (((rangeX2 - rangeX1) / self.dotclockDivider()) + 2) & ~3u;
    displayOutputActive = displayPixelsRemaining != 0;
    if(!accurate) {
      this->rangeX1 = rangeX1;
      divider = self.dotclockDivider();
      disabled = self.io.displayDisable;
    }
  }

  if(accurate && rangeX1 < rangeX2) {
    u32 lineStartX = sourceStartX;
    u32 sourceX = (lineStartX * 2 + srcPixelX * (depth == 0 ? 2 : 3)) & 2047;
    if(depth == 0) {
      prevPixelColor = 1 << 24 | (self.vram.readByte(sourceBase + sourceX)
                     | self.vram.readByte(sourceBase + ((sourceX + 1) & 2047)) << 8) & 0x7fff;
    } else {
      prevPixelColor = readRGB24(sourceX);
    }
    srcPixelX++;
    prevPixelStart = 0;
    hasPrevPixel = true;
  }
}

auto GPU::Scanout::clock(u32 hclock) -> void {
  if(!lineVisible || displayCountLatched) return;

  u32 rangeX1 = self.io.displayRangeX1;
  u32 rangeX2 = self.io.displayRangeX2;
  if(hclock < rangeX1) return;

  sourceStartX = self.io.displayStartX;
  srcPixelX = 0;
  hasPrevPixel = false;
  displayCountLatched = true;
  displayPixelsRemaining = rangeX2 > rangeX1 ? (((rangeX2 - rangeX1) / self.dotclockDivider()) + 2) & ~3u : 0;
  displayOutputActive = displayPixelsRemaining != 0;
  if(!accurate) {
    this->rangeX1 = rangeX1;
    divider = self.dotclockDivider();
    disabled = self.io.displayDisable;
    return;
  }
  if(self.io.dotcounter != (s32)self.dotclockDivider()) pixel(hclock);
}

auto GPU::Scanout::renderLine() -> void {
  if(!displayCountLatched || !displayOutputActive || targetY >= OutputHeight) return;

  u32 first = max(rangeX1, 488u);
  u32 last = pal ? 3300 : 3288;
  if(first >= last || !divider) return;

  u32 firstX = (first - 488) / ClockScale;
  u32 lastX = (last - 488) / ClockScale;
  u32 hclock = 488 + firstX * ClockScale + ClockScale - 1;
  if(hclock < rangeX1) {
    firstX++;
    hclock += ClockScale;
  }
  if(firstX >= lastX) return;

  u32 startPixel = rangeX1 / divider;
  u32 sourcePixel = hclock / divider - startPixel;
  u32 phase = hclock % divider;
  u32 pixelBytes = depth == 0 ? 2 : 3;
  u32 sourceX = (sourceStartX * 2 + sourcePixel * pixelBytes) & 2047;
  u32 blank = depth == 0 ? 1u << 24 : 0u;
  for(u32 x = firstX; x < lastX; x++) {
    if(sourcePixel >= displayPixelsRemaining) break;
    u32 color = disabled ? blank : depth == 0
      ? 1u << 24 | (self.vram2D[sourceBase >> 11][sourceX >> 1] & 0x7fff)
      : readRGB24(sourceX);
    outputPtr[x] = color;
    if(!interleaved && targetY + 1 < OutputHeight) outputPtr[OutputPitch + x] = color;
    phase += ClockScale;
    if(phase >= divider) {
      phase -= divider;
      sourcePixel++;
      sourceX = (sourceX + pixelBytes) & 2047;
    }
  }
}

alwaysinline auto GPU::Scanout::flushPixel(u32 hclock) -> void {
  if(!lineVisible || !hasPrevPixel || !displayOutputActive) return;
  u32 activeX1 = 488;
  u32 activeX2 = pal ? 3300 : 3288;
  u32 start = max(prevPixelStart, (u32)self.io.displayRangeX1);
  start = max(start, activeX1);
  u32 end = min(hclock, activeX2);
  if(end > start && targetY < OutputHeight) {
    u32 first = (start - activeX1) / ClockScale;
    u32 last = (end - activeX1) / ClockScale;
    u32 color = self.io.displayDisable ? (depth == 0 ? 1u << 24 : 0u) : prevPixelColor;
    if(last > first) {
      if(!interleaved && targetY + 1 < OutputHeight) {
        for(u32 x = first; x < last; x++) {
          outputPtr[x] = color;
          outputPtr[OutputPitch + x] = color;
        }
      } else {
        memory::fill<u32>(outputPtr + first, last - first, color);
      }
    }
  }
  prevPixelStart = hclock;
}

alwaysinline auto GPU::Scanout::pixel(u32 hclock) -> void {
  u32 rangeX1 = self.io.displayRangeX1;
  if(hclock < rangeX1) {
    if(hasPrevPixel) prevPixelStart = hclock;
    return;
  }
  if(!displayOutputActive) return;

  bool consume = hasPrevPixel && hclock > prevPixelStart;
  flushPixel(hclock);
  if(consume) {
    if(displayPixelsRemaining) --displayPixelsRemaining;
    if(!displayPixelsRemaining) displayOutputActive = false;
  }
  if(!displayOutputActive) return;

  u32 lineStartX = sourceStartX;
  u32 sourceX = (lineStartX * 2 + srcPixelX * (depth == 0 ? 2 : 3)) & 2047;
  if(depth == 0) {
    prevPixelColor = 1 << 24 | (self.vram2D[sourceBase >> 11][sourceX >> 1] & 0x7fff);
  } else {
    prevPixelColor = readRGB24(sourceX);
  }

  prevPixelStart = hclock;
  hasPrevPixel = true;
  srcPixelX++;
}

auto GPU::Scanout::lineEnd() -> void {
  if(!lineVisible) return;
  if(accurate) flushPixel(pal ? 3300 : 3288);
  else renderLine();

  hasPrevPixel = false;
  displayOutputActive = false;
  displayPixelsRemaining = 0;
}

auto GPU::Scanout::power() -> void {
  depth = 0;
  dotclock = 10;
  rangeY1 = 16;
  rangeY2 = 256;
  interlace = false;
  interleaved = false;
  pal = false;
  accurate = false;
  disabled = true;
  rangeX1 = 0;
  divider = 10;
  lineVisible = false;
  targetY = 0;
  sourceBase = 0;
  sourceStartX = 0;
  srcPixelX = 0;
  outputPtr = nullptr;
  displayCountLatched = false;
  displayOutputActive = false;
  displayPixelsRemaining = 0;
  hasPrevPixel = false;
  prevPixelStart = 0;
  prevPixelColor = 0;
}
