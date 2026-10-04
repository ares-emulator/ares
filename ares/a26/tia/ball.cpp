auto TIA::ObjectPipeline::Ball::reset(u8 counter) -> void {
  this->counter = counter;
  rendering = 1;
  renderCounter = -4 + (counter - 157);
}

auto TIA::ObjectPipeline::Ball::latchOutput() -> void {
  output = rendering && renderCounter >= 0 && enable[delay];
}

auto TIA::ObjectPipeline::Ball::clock(u8 clocks, n1 regularClock) -> void {
  while(clocks--) {
    latchOutput();
    auto starfield = moving && regularClock;

    if(counter == 156) {
      rendering = 1;
      renderCounter = -4;
      auto delta = (counter + 160 - lastMovementCounter) % 4;
      if(starfield && delta == 3 && (1 << size) < 4) renderCounter++;
      effectiveWidth = 1 << size;
      if(delta == 2) effectiveWidth = 0;
      if(delta == 3) effectiveWidth = (1 << size) == 1 ? 2 : 1 << size;
    } else if(rendering && ++renderCounter >= (starfield ? effectiveWidth : 1 << size)) {
      rendering = 0;
    }

    if(++counter == 160) counter = 0;
  }
}
