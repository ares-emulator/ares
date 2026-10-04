auto Video::clock(i16 x, n7 pixel, n1 hblank, n1 vblank) -> void {
  auto outputY = y();
  if(x < 0 || x >= 160 || outputY <= 0 || outputY >= displayHeight()) return;
  if(hblank || vblank) pixel = 0;
  screen->pixels().data()[outputY * 180 + x + 10] = pixel;
}

auto Video::fill(i16 x, n7 pixel) -> void {
  auto outputY = y();
  if(x >= 160 || outputY <= 0 || outputY >= displayHeight()) return;
  auto output = screen->pixels().data() + outputY * 180 + 10;
  for(auto outputX : range((u32)max(0, x), 160)) output[outputX] = pixel;
}

auto Video::endScanline() -> void {
  lineCounter++;
  linesSinceReturn++;

  if(sync == Sync::Pending && ++pulseLines >= 2) sync = Sync::Qualified;

  //Stella bounds missing sync at 675 lines; this is receiver policy.
  static constexpr u32 MissingSyncLines = 312 * 2 + 51;
  if(linesSinceReturn >= MissingSyncLines) fallback();
}

auto Video::vsync(n1 level) -> void {
  if(level) {
    if(sync == Sync::Waiting) {
      sync = Sync::Pending;
      pulseLines = 0;
    }
    return;
  }

  auto accepted = sync == Sync::Qualified;
  sync = Sync::Waiting;
  pulseLines = 0;
  if(accepted) accept();
}

auto Video::frame() -> void {
  controllerPort1.frame();
  controllerPort2.frame();
  scheduler.exit(Event::Frame);
}

auto Video::accept() -> void {
  auto frameLines = lineCounter;
  lineCounter = 0;
  linesSinceReturn = 0;
  screen->refreshRateHint(system.frequency(), 228, frameLines);
  screen->frame();
  frame();
}

auto Video::fallback() -> void {
  linesSinceReturn = 0;
  frame();
}
