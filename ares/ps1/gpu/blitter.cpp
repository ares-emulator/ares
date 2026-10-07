auto GPU::Blitter::queue() -> void {
  // The refresh callback owns this snapshot until the previous frame completes.
  self.screen->synchronize();
  self.renderer.synchronize();
  self.refreshed = true;

  self.screen->refreshRateHint(self.io.videoMode ? 50 : 60); // TODO: More accurate refresh rate hint

  //if the display is disabled, output a black screen image
  if(blank = self.io.displayDisable) {
    self.screen->frame();
    scheduler.exit(Event::Frame);
    return;
  }

  depth = self.io.colorDepth;

  // Capture geometry from the same current mode as the origin and color depth.
  // The scanline timing cache may still describe the previous frame.
  const u32 dotclock = self.dotclockDivider();
  const bool interlace = self.io.verticalResolution && self.io.interlace;
  width  = self.displayWidth();
  height = (self.io.videoMode ? 256 : 240) << interlace;

  s32 offsetX1 = 0;
  if(width == 256) offsetX1 = 0x228;
  if(width == 320) offsetX1 = 0x260;
  if(width == 368) offsetX1 = 0x260;  //untested
  if(width == 512) offsetX1 = 0x260;
  if(width == 640) offsetX1 = 0x260;

  s32 offsetY1 = 0;
  if(height == 240) offsetY1 = 0x10;
  if(height == 256) offsetY1 = 0x20;
  if(height == 480) offsetY1 = 0x10;
  if(height == 512) offsetY1 = 0x20;

  //target coordinates
  tx = max(((s32)self.io.displayRangeX1 - offsetX1) / (s32)dotclock, 0);
  ty = max(((s32)self.io.displayRangeY1 - offsetY1) << interlace, 0);
  tw = max(((s32)self.io.displayRangeX2 - (s32)self.io.displayRangeX1) / (s32)dotclock & ~3, 0);
  th = max(((s32)self.io.displayRangeY2 - (s32)self.io.displayRangeY1) << interlace, 0);

  // Clip against the actual viewport, not the 1024-pixel VRAM row width.
  tx = std::min<s32>(tx, width);
  ty = std::min<s32>(ty, height);
  tw = std::min<s32>(tw, width - tx);
  th = std::min<s32>(th, height - ty);

  self.display.previous.x = tx;
  self.display.previous.y = ty;
  self.display.previous.width  = tw;
  self.display.previous.height = th;

  //source coordinates
  sx = self.io.displayStartX;
  sy = self.io.displayStartY;

  //Refresh may be called from another thread: we need to make a copy of the display area for it to use
  self.vram.mutex.lock();
  auto bytesPerRow = width * (depth == 0 ? 2 : 3);
  for(int y = 0; y < height; y++) {
    u32 wrappedY = (y + sy) % 512;
    u32 startOffset = wrappedY * 1024 * 2 + sx * 2;
    u32 remainingInRow = 1024 * 2 - (startOffset % (1024 * 2));

    if(bytesPerRow > remainingInRow) {
      memory::copy(vram.data + startOffset, self.vram.data + startOffset, remainingInRow);

      u32 remainder = bytesPerRow - remainingInRow;
      memory::copy(vram.data + (startOffset + remainingInRow) % (1024 * 512 * 2),
                   self.vram.data + (startOffset + remainingInRow) % (1024 * 512 * 2),
                   remainder);
      continue;
    }

    memory::copy(vram.data + startOffset, self.vram.data + startOffset, bytesPerRow);
  }

  self.vram.mutex.unlock();

  self.screen->setViewport(0, 0, width, height);
  self.screen->frame();
  scheduler.exit(Event::Frame);
}

auto GPU::Blitter::refresh() -> void {
  auto pixels = self.screen->pixels(1);
  std::fill(pixels.begin(), pixels.end(), 0);
  if(blank) return;
  auto output = pixels.data();

  //15bpp
  if(depth == 0) {
    for(u32 y : range(th)) {
      if(y + ty < 0 || y + ty >= 512) continue;
      u32 source = (y + sy) * 1024 * 2 + sx * 2;
      u32 target = (y + ty) *  640 * 1 + tx * 1;
      for(u32 x : range(tw)) {
        u16 data = vram.readHalf(source);
        output[target++] = 1 << 24 | data & 0x7fff;
        source += 2;
      }
    }
  }

  //24bpp
  if(depth == 1) {
    for(u32 y : range(th)) {
      if(y + ty < 0 || y + ty >= 512) continue;
      u32 source = (y + sy) * 1024 * 2 + sx * 2;
      u32 target = (y + ty) *  640 * 1 + tx * 1;
      for(u32 x : range(tw)) {
        u32 data = vram.readWordUnaligned(source);
        output[target++] = data & 0xffffff;
        source += 3;
      }
    }
  }
}

auto GPU::Blitter::power() -> void {
  vram.allocate(1_MiB);
}
