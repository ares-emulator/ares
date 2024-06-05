template<u32 Cycle>
auto PPU::cycleSpriteEvaluation() -> void {
  if constexpr(Cycle == 0) return;

  if constexpr(Cycle > 0 && Cycle <= 64 && (Cycle & 1) == 1) {
    sprite.io.oamData = 0xff;
    return;
  }

  if constexpr(Cycle > 0 && Cycle <= 64 && (Cycle & 1) == 0) {
    soam[sprite.io.oamTempCounter++] = sprite.io.oamData;
    return;
  }

  if constexpr(Cycle > 64 && Cycle <= 256 && (Cycle & 1) == 1) {
    sprite.io.oamData = oam[sprite.io.oamMainCounter];
    return;
  }

  if constexpr(Cycle > 64 && Cycle <= 256 && (Cycle & 1) == 0) {
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
      return;
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
      return;
    }
  }

  if constexpr(Cycle > 256 && Cycle <= 320) {
    u32 index  = ((io.lx - 257) >> 3) << 2 + min((io.lx - 257) & 7, 3);

    sprite.io.oamMainCounter = 0;
    sprite.io.oamTempCounter = 0;
    sprite.io.oamMainCounterOverflow = false;
    sprite.io.oamTempCounterOverflow = false;
    sprite.io.oamData = soam[index];
    return;
  }

  if constexpr(Cycle > 320 && Cycle <= 340) {
    sprite.io.oamData = soam[0];
    return;
  }

  unreachable;
}

auto PPU::SpriteEvaluation::main() -> void {
  // skip in vblank
  if (!ppu.enable()) return;

  if (ppu.io.ly == ppu.vlines() - 1) {
    // In https://www.nesdev.org/wiki/PPU_sprite_evaluation,
    // Sprite evaluation does not happen on the pre-render
    // scanline. Because evaluation applies to the next
    // line's sprite rendering, no sprites will be rendered
    // on the first scanline, and this is why there is a 1
    // line offset on a sprite's Y coordinate.
    if (ppu.io.lx > 0 && ppu.io.lx <= 8) {
      if (io.oamMainCounterIndex >= 2) {
        ppu.oam[ppu.io.lx - 1] = ppu.oam[(io.oamMainCounter & 0xf8) + ppu.io.lx - 1];
      }
    }
    return;
  }

#define cycle(n) if (ppu.io.lx == (n)) return ppu.cycleSpriteEvaluation<n>()
#define cycle02(n) cycle(n); cycle(n + 1)
#define cycle04(n) cycle02(n); cycle02(n + 2)
#define cycle08(n) cycle04(n); cycle04(n + 4)
#define cycle16(n) cycle08(n); cycle08(n + 8)
#define cycle32(n) cycle16(n); cycle16(n + 16)
#define cycle64(n) cycle32(n); cycle32(n + 32)
#define cycle128(n) cycle64(n); cycle64(n + 64)
#define cycle256(n) cycle128(n); cycle128(n + 128)
  cycle256(0);
  cycle64(256);
  cycle16(320);
  cycle04(336);
}
