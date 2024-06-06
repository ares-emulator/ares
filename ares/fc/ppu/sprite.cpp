auto PPU::cycleSpriteEvaluation() -> void {
  if (io.lx == 0) return;

  if (io.lx <= 64) {
    if ((io.lx & 1) == 1)
      sprite.io.oamData = 0xff;
    else
      soam[sprite.io.oamTempCounter++] = sprite.io.oamData;
    return;
  }

  if (io.lx <= 256) {
    if ((io.lx & 1) == 1) {
      sprite.io.oamData = oam[sprite.io.oamMainCounter];
      return;
    }

    if (sprite.io.oamMainCounterOverflow) {
      ++sprite.io.oamMainCounterIndex;
      if (sprite.io.oamTempCounterOverflow)
        sprite.io.oamData = soam[sprite.io.oamTempCounter];
      return;
    }

    if (sprite.io.oamTempCounterTiming == 0) {
      s32 ly = io.ly == vlines() - 1 ? -1 : (s32)io.ly;
      u32 y = ly - sprite.io.oamData;

      if (sprite.io.oamTempCounterOverflow) {
        sprite.io.oamData = soam[sprite.io.oamTempCounter];

        if (y >= io.spriteHeight) {
          if (++sprite.io.oamMainCounterIndex == 0)
            sprite.io.oamMainCounterOverflow = true;
          ++sprite.io.oamMainCounterTiming;
          return;
        }

        sprite.io.spriteOverflow = 1;
        ++sprite.io.oamTempCounter;
        ++sprite.io.oamMainCounterTiming;
        return;
      }

      soam[sprite.io.oamTempCounter] = sprite.io.oamData;
      if (y >= io.spriteHeight) {
        if (++sprite.io.oamMainCounterIndex == 0)
          sprite.io.oamMainCounterOverflow = true;
        return;
      }

      latch.oamId[sprite.io.oamTempCounterIndex] = sprite.io.oamMainCounterIndex;
      ++sprite.io.oamTempCounter;
      ++sprite.io.oamMainCounterTiming;
    } else {
      if (sprite.io.oamTempCounterOverflow) {
        sprite.io.oamData = soam[sprite.io.oamTempCounter];
        ++sprite.io.oamTempCounter;
        ++sprite.io.oamMainCounterTiming;
        return;
      }

      soam[sprite.io.oamTempCounter] = sprite.io.oamData;
      ++sprite.io.oamTempCounter;
      ++sprite.io.oamMainCounterTiming;

      if (sprite.io.oamTempCounterTiming == 0) {
        if (++sprite.io.oamMainCounterIndex == 0)
          sprite.io.oamMainCounterOverflow = true;

        if (sprite.io.oamTempCounter == 0)
          sprite.io.oamTempCounterOverflow = true;
      }
    }
    return;
  }

  if (io.lx <= 320) {
    u32 index  = ((io.lx - 257) >> 3) << 2 + min((io.lx - 257) & 7, 3);

    sprite.io.oamMainCounter = 0;
    sprite.io.oamTempCounter = 0;
    sprite.io.oamMainCounterOverflow = false;
    sprite.io.oamTempCounterOverflow = false;
    sprite.io.oamData = soam[index];
    return;
  }

  if (io.lx <= 340) {
    sprite.io.oamData = soam[0];
    return;
  }
}

auto PPU::cyclePrepareSpriteEvaluation() -> void {
  // In https://www.nesdev.org/wiki/PPU_sprite_evaluation,
  // Sprite evaluation does not happen on the pre-render
  // scanline. Because evaluation applies to the next
  // line's sprite rendering, no sprites will be rendered
  // on the first scanline, and this is why there is a 1
  // line offset on a sprite's Y coordinate.
  if (io.lx > 0 && io.lx <= 8) {
    if (sprite.io.oamMainCounterIndex >= 2) {
      oam[io.lx - 1] = oam[(sprite.io.oamMainCounter & 0xf8) + io.lx - 1];
    }
  }
}
