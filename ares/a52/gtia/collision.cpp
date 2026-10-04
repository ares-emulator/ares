auto GTIA::Collision::clock(int playfield, u8 players, u8 missiles) -> void {
  if(playfield >= 0) {
    for(u32 index = 0; index < 4; index++) {
      if(missiles >> index & 1) missilePlayfield[index] |= 1 << playfield;
      if(players >> index & 1) playerPlayfield[index] |= 1 << playfield;
    }
  }
  for(u32 missile = 0; missile < 4; missile++) {
    if(!(missiles >> missile & 1)) continue;
    missilePlayer[missile] |= players;
  }
  for(u32 player = 0; player < 4; player++) {
    if(!(players >> player & 1)) continue;
    playerPlayer[player] |= players & ~(1 << player);
  }
}

auto GTIA::Collision::read(u8 address) const -> n8 {
  if(address <= 0x03) return missilePlayfield[address];
  if(address <= 0x07) return playerPlayfield[address - 0x04];
  if(address <= 0x0b) return missilePlayer[address - 0x08];
  return playerPlayer[address - 0x0c];
}

auto GTIA::Collision::clear() -> void {
  *this = {};
}
