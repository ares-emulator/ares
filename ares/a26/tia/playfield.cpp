auto TIA::Playfield::clock(i16 x) -> n1 {

  //HMOVE HBLANK still clocks visible x=0..7.
  if(x < 0 || x >= 160) return pixel;

  //Latch REF before drawing each half.
  if(x == 0 || x == 79) mirrorActive = mirror;

  if((x % 4) == 0) {
    auto pos = x >> 2;
    pixel = (!mirrorActive || pos < 20) ? graphics.bit(pos % 20) : graphics.bit(19 - (pos % 20));
  }

  return pixel;
}

auto TIA::Playfield::nextLine() -> void {
  pixel = 0;
}
