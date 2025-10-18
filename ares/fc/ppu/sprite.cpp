auto PPU::cycleSpriteEvaluation() -> void {
  if (io.lx == 0) return;

  if (io.lx <= 64) {
    if ((io.lx & 1) == 1) {
      sprite.oamData = 0xff;
    } else {
      soam[sprite.oamTempCounter++] = sprite.oamData;
    }
    return;
  }

  if (io.lx <= 256) {
    if ((io.lx & 1) == 1) {
      sprite.oamData = oam[sprite.oamMainCounter];
      return;
    }

    if (sprite.oamMainCounterOverflow) {
      ++sprite.oamMainCounterIndex;
      if (sprite.oamTempCounterOverflow) sprite.oamData = soam[sprite.oamTempCounter];
      return;
    }

    s32 ly = io.ly == vlines() - 1 ? -1 : (s32)io.ly;
    u32 y = ly - sprite.oamData;

    if (sprite.oamTempCounterOverflow) {
      sprite.oamData = soam[sprite.oamTempCounter];
    } else {
      soam[sprite.oamTempCounter] = sprite.oamData;
    }

    if (sprite.oamTempCounterTiming == 0) {
      if (sprite.oamTempCounterOverflow) {
        if (y >= io.spriteHeight) {
          if (++sprite.oamMainCounterIndex == 0) sprite.oamMainCounterOverflow = true;
          ++sprite.oamMainCounterTiming;
          return;
        }

        sprite.spriteOverflow = 1;
      } else {
        if (y >= io.spriteHeight) {
          sprite.oamMainCounterTiming = 0;
          if (++sprite.oamMainCounterIndex == 0) sprite.oamMainCounterOverflow = true;
          return;
        }

        // first sprite evaluated should be treated as sprite zero
        latch.oamId[sprite.oamTempCounterIndex] = (io.lx == 66) ? 0 : 1;
      }
      ++sprite.oamTempCounter;
      if (++sprite.oamMainCounter == 0) sprite.oamMainCounterOverflow = true;
    } else {
      ++sprite.oamTempCounter;
      if (++sprite.oamMainCounter == 0) sprite.oamMainCounterOverflow = true;
      if (sprite.oamTempCounter == 0) sprite.oamTempCounterOverflow = true;
      if (sprite.oamTempCounterTiming == 0 && y >= io.spriteHeight) sprite.oamMainCounterTiming = 0;
    }
    return;
  }

  if (io.lx <= 320) {
    u32 index  = (((io.lx - 257) >> 3) << 2) + min((io.lx - 257) & 7, 3);

    sprite.oamMainCounter = 0;
    sprite.oamTempCounter = 0;
    sprite.oamMainCounterOverflow = false;
    sprite.oamTempCounterOverflow = false;
    sprite.oamData = soam[index];
    return;
  }

  if (io.lx <= 340) {
    sprite.oamData = soam[0];
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
    if (sprite.oamMainCounterIndex >= 2) {
      oam[io.lx - 1] = oam[(sprite.oamMainCounter & 0xf8) + io.lx - 1];
    }
  }
}
