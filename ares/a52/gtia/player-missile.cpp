auto GTIA::loadPlayerDMA(u8 player, n8 data, u32 scanline) -> void {
  playerMissile.loadPlayerDMA(player, data, graphicsControl.bit(1), scanline & 1);
}

auto GTIA::loadMissileDMA(n8 data, u32 scanline) -> void {
  playerMissile.loadMissileDMA(data, graphicsControl.bit(0), scanline & 1);
}

auto GTIA::PlayerMissile::loadPlayerDMA(u8 player, n8 data, bool enabled, bool oddLine) -> void {
  if(player >= 4 || !enabled) return;
  if(verticalDelay.bit(player + 4) && !oddLine) return;
  playerGraphics[player] = data;
}

auto GTIA::PlayerMissile::loadMissileDMA(n8 data, bool enabled, bool oddLine) -> void {
  if(!enabled) return;
  u8 mask = 0xff;
  for(u32 missile = 0; missile < 4; missile++) {
    if(verticalDelay.bit(missile) && !oddLine) mask &= ~(3 << (missile * 2));
  }
  data = ((u8)missileGraphics & ~mask) | ((u8)data & mask);
  missileGraphics = data;
}

auto GTIA::PlayerMissile::clock(u8 horizontal) -> Signals {
  u8 players = 0;
  u8 missiles = 0;

  for(u32 index = 0; index < 4; index++) {
    if(clockPlayer(index, horizontal)) players |= 1 << index;
    if(clockMissile(index, horizontal)) missiles |= 1 << index;
  }

  return {players, missiles};
}

auto GTIA::PlayerMissile::clockPlayer(u8 index, u8 horizontal) -> bool {
  if(horizontal == playerPosition[index]) {
    playerShift[index] |= playerGraphics[index];
    playerRemaining[index] = 8;
    playerStretch[index] = 0;
  }

  bool output = playerRemaining[index] && playerShift[index].bit(7);
  if(playerRemaining[index]) {
    playerStretch[index] = ((u8)playerStretch[index] + 1) & (u8)playerSize[index];
    if(!playerStretch[index]) {
      playerShift[index] <<= 1;
      playerRemaining[index]--;
    }
  }
  return output;
}

auto GTIA::PlayerMissile::clockMissile(u8 index, u8 horizontal) -> bool {
  if(horizontal == missilePosition[index]) {
    missileShift[index] |= (u8)missileGraphics >> (index * 2) & 3;
    missileRemaining[index] = 2;
    missileStretch[index] = 0;
  }

  bool output = missileRemaining[index] && missileShift[index].bit(1);
  auto sizeCode = (u8)missileSize >> (index * 2) & 3;
  if(missileRemaining[index]) {
    missileStretch[index] = ((u8)missileStretch[index] + 1) & sizeCode;
    if(!missileStretch[index]) {
      missileShift[index] <<= 1;
      missileRemaining[index]--;
    }
  }
  return output;
}

auto GTIA::PlayerMissile::clockRegisters() -> void {
  for(u32 index = 0; index < 8; index++) {
    if(positionDelay[index] && !--positionDelay[index]) {
      if(index < 4) playerPosition[index] = pendingPosition[index];
      else missilePosition[index - 4] = pendingPosition[index];
    }
  }
  for(u32 index = 0; index < 5; index++) {
    if(graphicsDelay[index] && !--graphicsDelay[index]) {
      if(index < 4) playerGraphics[index] = pendingGraphics[index];
      else missileGraphics = pendingGraphics[index];
    }
  }
  for(u32 index = 0; index < 4; index++) {
    if(playerSizeDelay[index] && !--playerSizeDelay[index]) {
      playerSize[index] = pendingPlayerSize[index];
    }
  }
  if(missileSizeDelay && !--missileSizeDelay) missileSize = pendingMissileSize;
}

auto GTIA::PlayerMissile::writePosition(u8 index, n8 data) -> void {
  if(index >= 8) return;
  pendingPosition[index] = data;
  positionDelay[index] = 5;
}

auto GTIA::PlayerMissile::writeGraphics(u8 index, n8 data) -> void {
  if(index >= 5) return;
  pendingGraphics[index] = data;
  graphicsDelay[index] = 3;
}

auto GTIA::PlayerMissile::writePlayerSize(u8 index, n2 data) -> void {
  if(index >= 4) return;
  pendingPlayerSize[index] = data;
  playerSizeDelay[index] = 5;
}

auto GTIA::PlayerMissile::writeMissileSize(n8 data) -> void {
  pendingMissileSize = data;
  missileSizeDelay = 5;
}

auto GTIA::PlayerMissile::writeVerticalDelay(n8 data) -> void {
  verticalDelay = data;
}
