auto PPU::Timer::step() -> bool { 
  n16 counterNext = counter - 1;
  if(enable && counter) {
    counter = counterNext;
    if(repeat && !counter) {
      counter = frequency;
    }
  }
  return counterNext == 0;
}
