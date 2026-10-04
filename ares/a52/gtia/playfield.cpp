auto GTIA::Playfield::clock(n3 anValue) -> Input {
  Input input = {};
  auto an = (u8)anValue;

  if(an == 2 || an == 3) {
    input.synchronize = !horizontalBlank;
    horizontalBlank = 1;
    highResolution = an == 3;
  } else {
    horizontalBlank = 0;
  }

  if(an == 1) {
    input.sample[0].vsync = 1;
    input.sample[1].vsync = 1;
  } else if(an == 2 || an == 3) {
    input.sample[0].hblank = 1;
    input.sample[1].hblank = 1;
  } else if(an >= 4) {
    input.bit[0] = an >> 1 & 1;
    input.bit[1] = an >> 0 & 1;
    if(highResolution) {
      input.sample[0].an = 5 + (an >> 1 & 1);
      input.sample[1].an = 5 + (an >> 0 & 1);
    } else {
      input.sample[0].an = an - 3;
      input.sample[1].an = an - 3;
    }
  }

  return input;
}

auto GTIA::Playfield::signals(Sample sample, u8 special, u8 mode) const -> Signals {
  int collisionPlayfield = -1;
  u8 playfields = 0;
  u8 specialPlayers = 0;
  auto an = (u8)sample.an;
  if(an >= 1 && an <= 4) {
    collisionPlayfield = an - 1;
    playfields = 1 << collisionPlayfield;
  }

  if(!mode && (an == 5 || an == 6)) {
    // High-resolution data always enters priority as PF2. The two half-color
    // clock pixels are ORed for collision purposes, so only a set bit latches
    // the PF2 collision.
    playfields = 1 << 2;
    if(an == 6) collisionPlayfield = 2;
  } else if(special != 0xff) {
    collisionPlayfield = -1;
    playfields = 0;
    if(mode == 2) {
      if(special <= 3) {
        specialPlayers = 1 << special;
      } else if((special >= 4 && special <= 7) || special >= 12) {
        collisionPlayfield = special & 3;
        playfields = 1 << collisionPlayfield;
      }
    }
  }

  return {collisionPlayfield, playfields, specialPlayers};
}

auto GTIA::Playfield::clearHighResolution() -> void {
  highResolution = 0;
}

auto GTIA::Playfield::power() -> void {
  *this = {};
  horizontalBlank = 1;
}
