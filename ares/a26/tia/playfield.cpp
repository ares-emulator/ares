auto TIA::runPlayfield(n8 x) -> n1 {

  if((x % 4) == 0) {
    auto pos = x >> 2;
    playfield.pixel = (!playfield.mirror || pos < 20) ? playfield.graphics.bit(pos % 20)
                                                      : playfield.graphics.bit(19 - (pos % 20));
  }

  return playfield.pixel;
}
