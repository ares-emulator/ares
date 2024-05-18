auto PPU::SpriteEvaluation::load() -> void {
  oam.allocate(256);
}

auto PPU::SpriteEvaluation::unload() -> void {
  oam.reset();
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
        oam[ppu.io.lx - 1] = oam[(io.oamMainCounter & 0xf8) + ppu.io.lx - 1];
      }
    }
    return;
  }

  if (ppu.io.lx == 0) {
    io.oamTempCounter = 0;
    return;
  }

  // (0, 64]
  // set secondary OAM to 0xff
  if (ppu.io.lx > 0 && ppu.io.lx <= 64) {
    if (ppu.io.lx & 1) {
      io.oamData = 0xff;
      return;
    }

    soam[io.oamTempCounterIndex][io.oamTempCounterTiming] = io.oamData;
    ++io.oamTempCounter;
    return;
  }

  if (ppu.io.lx == 65) {
    io.oamMainCounterOverflow = false;

    io.oamTempCounter = 0;
    io.oamTempCounterOverflow = false;
  }
  
  // (64, 256]
  if (ppu.io.lx > 64 && ppu.io.lx <= 256) {
    if (ppu.io.lx & 1) {
      io.oamData = oam[io.oamMainCounter];
      return;
    }

    if (io.oamMainCounterOverflow) {
      ++io.oamMainCounterIndex;
      if (io.oamTempCounterOverflow)
        io.oamData = soam[io.oamTempCounterIndex][io.oamTempCounterTiming];
      return;
    }

    if (io.oamTempCounterTiming == 0) {
      s32 ly = ppu.io.ly == ppu.vlines() - 1 ? -1 : (s32)ppu.io.ly;
      u32 y = ly - io.oamData;

      if (io.oamTempCounterOverflow) {
        io.oamData = soam[io.oamTempCounterIndex][io.oamTempCounterTiming];

        if (y >= ppu.io.spriteHeight) {
          if (++io.oamMainCounterIndex == 0)
            io.oamMainCounterOverflow = true;
          ++io.oamMainCounterTiming;
          return;
        }

        io.spriteOverflow = 1;
        ++io.oamTempCounter;
        ++io.oamMainCounterTiming;
        return;
      }

      soam[io.oamTempCounterIndex].y = io.oamData;
      if (y >= ppu.io.spriteHeight) {
        if (++io.oamMainCounterIndex == 0)
          io.oamMainCounterOverflow = true;
        return;
      }

      soam[io.oamTempCounterIndex].id = io.oamMainCounterIndex;
      ++io.oamTempCounter;
      ++io.oamMainCounterTiming;
      return;
    } else {
      if (io.oamTempCounterOverflow) {
        io.oamData = soam[io.oamTempCounterIndex][io.oamTempCounterTiming];
        ++io.oamTempCounter;
        ++io.oamMainCounterTiming;
        return;
      }

      soam[io.oamTempCounterIndex][io.oamTempCounterTiming] = io.oamData;
      ++io.oamTempCounter;
      ++io.oamMainCounterTiming;

      if (io.oamTempCounterTiming == 0) {
        if (++io.oamMainCounterIndex == 0)
          io.oamMainCounterOverflow = true;

        if (io.oamTempCounter == 0)
          io.oamTempCounterOverflow = true;
      }
      return;
    }
  }

  if (ppu.io.lx == 257) {
    io.oamTempCounter = 0;
    io.oamTempCounterOverflow = false;
  }

  // (256, 320]
  if (ppu.io.lx > 256 && ppu.io.lx <= 320) {
    io.oamAddress = 0;

    if (ppu.io.lx & 1) {
      io.oamData = oam[io.oamMainCounter];
      return;
    }

    io.oamData = soam[io.oamTempCounterIndex][io.oamTempCounterTiming];
    ++io.oamTempCounter;
  }

  // (320, 336]
  if (ppu.io.lx > 320 && ppu.io.lx <= 336)
    io.oamData = soam[0][0];
}

auto PPU::SpriteEvaluation::power(bool reset) -> void {
  if (!reset) 
    oam.fill();

  io = {};

  for (auto& o : soam) o = {};
}
