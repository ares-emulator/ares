auto GTIA::frame() -> void {
  if(!screen) return;
  if(screen->overscan()) {
    screen->setSize(OverscanViewportWidth, OverscanViewportHeight);
    screen->setViewport(
      OverscanViewportX, OverscanViewportY,
      OverscanViewportWidth, OverscanViewportHeight
    );
  } else {
    screen->setSize(NominalViewportWidth, NominalViewportHeight);
    screen->setViewport(
      NominalViewportX, NominalViewportY,
      NominalViewportWidth, NominalViewportHeight
    );
  }
  screen->frame();
}

auto GTIA::outputSample(
  u32 y, u32 x, Sample sample, u8 players, u8 missiles, n1 specialBit
) -> void {
  if(y >= Timing::ScanlinesPerFrame || x >= Timing::SamplesPerScanline) return;

  auto mode = priority.mode();
  auto phase = x & 3;
  if(phase == 0) {
    if(playfield.special.y != y) playfield.special.mode10Previous = 0;
    playfield.special.y = y;
    playfield.special.x = x;
  }
  playfield.special.sample[phase] = sample;
  playfield.special.players[phase] = players;
  playfield.special.missiles[phase] = missiles;
  playfield.special.bit[phase] = specialBit;

  if(mode) {
    if(phase != 3) return;

    u8 value = 0;
    for(u32 index = 0; index < 4; index++) {
      value = value << 1 | playfield.special.bit[index];
    }
    bool lowResolutionBAKMute = mode == 2
      && (u8)playfield.special.sample[0].an >= 1
      && (u8)playfield.special.sample[0].an <= 4
      && (u8)playfield.special.sample[2].an == 0
      && (u8)playfield.special.sample[3].an == 0;
    if(lowResolutionBAKMute) value = 8;
    for(u32 index = 0; index < 4; index++) {
      // Mode 10 delays playfield identity by one color clock: the first half
      // uses the previous nibble and the second half uses the current nibble.
      // In low-resolution modes, BAK on the second color clock mutes both halves.
      auto delayed = mode == 2 && index < 2 && !lowResolutionBAKMute
        ? (u8)playfield.special.mode10Previous : value;
      auto color = resolve(playfield.special.sample[index], delayed,
        playfield.special.players[index], playfield.special.missiles[index]);
      if(screen) {
        screen->pixels().data()[
          playfield.special.y * Timing::SamplesPerScanline + playfield.special.x + index
        ] = color;
      }
    }
    if(mode == 2) playfield.special.mode10Previous = value;
    return;
  }

  auto color = resolve(sample, 0xff, players, missiles);
  if(screen) screen->pixels().data()[y * Timing::SamplesPerScanline + x] = color;
}

auto GTIA::resolve(Sample sample, u8 special, u8 players, u8 missiles) -> n8 {
  n8 playerColors[4];
  n8 playfieldColors[4];
  for(u32 index = 0; index < 4; index++) {
    playerColors[index] = colors.player(index);
    playfieldColors[index] = colors.playfield(index);
  }
  auto backgroundColor = (u8)colors.background();

  if(sample.vsync) return 0;
  if(sample.hblank) {
    return backgroundColor;
  }

  auto mode = priority.mode();
  auto playfieldSignals = playfield.signals(sample, special, mode);
  collision.clock(playfieldSignals.collision, players, missiles);

  players |= playfieldSignals.specialPlayers;
  bool fifthPlayer = priority.fifthPlayer();
  if(fifthPlayer) playfieldSignals.playfields |= missiles ? 1 << 3 : 0;
  else players |= missiles;

  n8 sourceColors[4] = {
    playfieldColors[0],
    playfieldColors[1],
    playfieldColors[2],
    playfieldColors[3],
  };
  if(special != 0xff) {
    if(mode == 1) {
      backgroundColor |= special;
      sourceColors[3] = (u8)playfieldColors[3] | special;
    }
    if(mode == 2) {
      if((special >= 4 && special <= 7) || special >= 12) {
        sourceColors[special & 3] = playfieldColors[special & 3];
      }
    }
    if(mode == 3) {
      backgroundColor = special ? (special << 4 | backgroundColor) : (backgroundColor & 0xf0);
      sourceColors[3] = special
        ? (special << 4 | (u8)playfieldColors[3])
        : ((u8)playfieldColors[3] & 0xf0);
    }
  }

  auto signals = priority.signals(players, playfieldSignals.playfields);
  // P5 normally enters the PF3 path, but its raw signal also suppresses the
  // other playfields. This creates the documented PRIOR=%1000 three-way case:
  // PF0/1 suppress the players and P5 then suppresses PF0/1.
  if(fifthPlayer && missiles && (playfieldSignals.playfields & 0x03) && players
  && priority.rule() == 0x08) {
    signals = {0, 0x08, false};
  }
  n8 color = 0;
  if(signals.background) color = backgroundColor;
  for(u32 index = 0; index < 4; index++) {
    if(signals.players >> index & 1) color = (u8)color | (u8)playerColors[index];
    if(signals.playfields >> index & 1) color = (u8)color | (u8)sourceColors[index];
  }

  // High-resolution data bypasses priority and impresses PF1 luminance onto
  // the selected source, including players, missiles, and the fifth player.
  if(!mode && (u8)sample.an == 6) {
    color = ((u8)color & 0xf0) | ((u8)playfieldColors[1] & 0x0f);
  }

  return color;
}
