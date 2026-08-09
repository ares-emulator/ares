auto GTIA::clock(n3 an) -> void {
  console.clock(graphicsControl);
  playerMissile.clockRegisters();
  priority.clock();
  colors.clock();

  auto input = playfield.clock(an);
  if(input.synchronize) counter.synchronizeHorizontalBlank();

  auto objects = playerMissile.clock(counter.horizontal);

  auto x = (u32)counter.horizontal * Timing::SamplesPerColorClock;
  auto y = (u32)counter.vertical;
  outputSample(y, x + 0, input.sample[0], objects.players, objects.missiles, input.bit[0]);
  outputSample(y, x + 1, input.sample[1], objects.players, objects.missiles, input.bit[1]);

  // Enabling a GTIA special mode clears the normal high-resolution latch.
  // Returning to normal mode cannot restore it until ANTIC presents the next
  // horizontal-blank mode code, which is the source of pseudo mode E.
  if(priority.mode()) playfield.clearHighResolution();
  counter.advance();
}

auto GTIA::Counter::synchronizeHorizontalBlank() -> void {
  horizontal = Timing::VisibleLastColorClock + 1;
}

auto GTIA::Counter::advance() -> void {
  if(++horizontal == Timing::ColorClocksPerScanline) {
    horizontal = 0;
    if(++vertical == Timing::ScanlinesPerFrame) vertical = 0;
  }
}

auto GTIA::Counter::power() -> void {
  *this = {};
  // ANTIC machine cycle 0 begins three color clocks before GTIA horizontal
  // position 0. The GTIA counter therefore finishes the preceding line first.
  horizontal = Timing::ColorClocksPerScanline - 3;
  vertical = Timing::ScanlinesPerFrame - 1;
}
