auto PPU::canAccessVRAM() const -> bool {
  if(!status.displayEnable) return 1;
  if(history.mode.bit(4,5) == 3) return 0;
  if(history.mode.bit(4,5) == 2 && status.lx >> 2 == 20) return 0;
  return 1;
}

auto PPU::canAccessOAM() const -> bool {
  if(!status.displayEnable) return 1;
  if(status.dmaActive && status.dmaClock >= 8) return 0;
  if(history.mode.bit(4,5) == 2) return 0;
  if(history.mode.bit(4,5) == 3) return 0;
  if(status.ly != 0 && status.lx >> 2 == 0) return 0;
  return 1;
}

auto PPU::compareLYC() const -> bool {
  auto ly  = status.ly;
  auto lyc = status.lyc;

  if(Model::GameBoy() || Model::SuperGameBoy()) {
    auto lx = status.lx >> 2;
    if(ly !=   0 && lx == 0) return 0;
    if(ly == 153 && lx == 2) return 0;
    if(ly == 153 && lx >= 3) return lyc == 0;
  }

  if(Model::GameBoyColor() && cpu.lowSpeed()) {
    auto lx = status.lx >> 2;
    if(ly == 153 && lx >= 1) return lyc == 0;
  }

  if(Model::GameBoyColor() && cpu.highSpeed()) {
    auto lx = status.lx >> 1;
    if(ly !=   0 && lx == 0) return lyc == ly - 1;
    if(ly == 153 && lx >= 8) return lyc == 0;
  }

  return lyc == ly;
}
