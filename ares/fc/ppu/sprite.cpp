auto PPU::SpriteEvaluation::main() -> void {
  if (!ppu.enable()) return;

  // skip in vblank
  if (ppu.io.ly >= 240 && ppu.io.ly < ppu.vlines() - 2) return;

  n16 x = ppu.io.lx - 1;

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

    soam[io.oamTempCounterIndex].data[io.oamTempCounterTiming] = io.oamData;
    ++io.oamTempCounter;
    return;
  }

  if (ppu.io.lx == 65) {
    io.oamMainCounter = io.oamAddress;
    io.oamMainCounterOverflow = false;

    io.oamMainCounter = 0;
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
        io.oamData = soam[io.oamTempCounterIndex].data[io.oamTempCounterTiming];
      return;
    }

    if (io.oamTempCounterTiming == 0) {
      s32 ly = ppu.io.ly == ppu.vlines() - 1 ? -1 : (s32)ppu.io.ly;
      u32 y = ly - io.oamData;

      if (io.oamTempCounterOverflow) {
        io.oamData = soam[io.oamTempCounterIndex].data[io.oamTempCounterTiming];

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
        io.oamData = soam[io.oamTempCounterIndex].data[io.oamTempCounterTiming];
        ++io.oamTempCounter;
        ++io.oamMainCounterTiming;
        return;
      }

      soam[io.oamTempCounterIndex].data[io.oamTempCounterTiming] = io.oamData;
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
}

auto PPU::SpriteEvaluation::power(bool reset) -> void {
  if (!reset) 
    oam.fill();

  io = {};

  for (auto& o : soam) o = {};
}
