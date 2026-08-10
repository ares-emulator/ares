auto GTIA::serialize(serializer& s) -> void {
  playerMissile.serialize(s);
  colors.serialize(s);
  priority.serialize(s);
  s(graphicsControl);
  console.serialize(s);
  collision.serialize(s);
  playfield.serialize(s);
  counter.serialize(s);
}

auto GTIA::PlayerMissile::serialize(serializer& s) -> void {
  for(auto& value : playerPosition) s(value);
  for(auto& value : missilePosition) s(value);
  for(auto& value : playerSize) s(value);
  s(missileSize);
  for(auto& value : playerGraphics) s(value);
  s(missileGraphics);
  s(verticalDelay);

  for(auto& value : playerShift) s(value);
  for(auto& value : missileShift) s(value);
  for(auto& value : playerRemaining) s(value);
  for(auto& value : missileRemaining) s(value);
  for(auto& value : playerStretch) s(value);
  for(auto& value : missileStretch) s(value);

  for(auto& value : pendingPosition) s(value);
  for(auto& value : positionDelay) s(value);
  for(auto& value : pendingGraphics) s(value);
  for(auto& value : graphicsDelay) s(value);
  for(auto& value : pendingPlayerSize) s(value);
  for(auto& value : playerSizeDelay) s(value);
  s(pendingMissileSize);
  s(missileSizeDelay);
}

auto GTIA::ColorRegisters::serialize(serializer& s) -> void {
  for(auto& value : playerColor) s(value);
  for(auto& value : playfieldColor) s(value);
  s(backgroundColor);
  for(auto& value : pendingColor) s(value);
  for(auto& value : colorDelay) s(value);
}

auto GTIA::Priority::serialize(serializer& s) -> void {
  s(control);
  s(pendingLow);
  s(lowDelay);
  s(pendingMode);
  s(modeDelay);
}

auto GTIA::Console::serialize(serializer& s) -> void {
  s(output);
  s(pinSense);
  for(auto& value : trigger) s(value);
}

auto GTIA::Collision::serialize(serializer& s) -> void {
  for(auto& value : missilePlayfield) s(value);
  for(auto& value : playerPlayfield) s(value);
  for(auto& value : missilePlayer) s(value);
  for(auto& value : playerPlayer) s(value);
}

auto GTIA::Playfield::serialize(serializer& s) -> void {
  for(auto& sample : special.sample) {
    s(sample.an);
    s(sample.hblank);
    s(sample.vsync);
  }
  for(auto& value : special.players) s(value);
  for(auto& value : special.missiles) s(value);
  for(auto& value : special.bit) s(value);
  s(special.y);
  s(special.x);
  s(special.mode10Previous);
  s(highResolution);
  s(horizontalBlank);
}

auto GTIA::Counter::serialize(serializer& s) -> void {
  s(horizontal);
  s(vertical);
}
