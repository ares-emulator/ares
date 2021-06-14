auto PPU::Timer::step() -> bool {
  if(enable && counter && !--counter) {
    if(repeat) counter = frequency;
    return true;
  }
  return false;
}
