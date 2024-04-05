auto TIA::Ball::reset() -> void {
  counter = -4; // Resetting takes 4 cycles
}

auto TIA::Ball::step(u8 clocks) -> void {
  if(!clocks) return;
  while(clocks--) {
    if(++counter == 160) {
      counter = 0;
    }

    const int ballSizes[4] = {1, 2, 4, 8};
    output = enable[delay] && counter < ballSizes[size];
  }
}

