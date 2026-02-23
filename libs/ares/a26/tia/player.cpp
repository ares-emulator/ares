auto TIA::Player::reset() -> void {
  counter = -4;
}

auto TIA::Player::start(n1 copy) -> void {
  this->copy = copy;
  startCounter = 5;
  starting = true;
}

auto TIA::Player::width() -> u8 {
  switch(size) {
    case 5:  return 2;
    case 7:  return 4;
    default: return 1;
  }
}

auto TIA::Player::stepPixelCounter() -> void {
  if(--widthCounter == 0) {
    pixelCounter--;
    widthCounter = width();
  }

  if(!copy && missile.lockedToPlayer && pixelCounter == 4) {
    missile.reset();
  }
}

auto TIA::Player::step(u8 clocks) -> void {
  if(!clocks) return;
  while(clocks--) {
    counter++;

    n1 first  = size == 1 || size == 3;
    n1 second = size == 2 || size == 3 || size == 6;
    n1 third  = size == 4 || size == 6;

    if(first  && counter == 12) start(true);
    if(second && counter == 28) start(true);
    if(third  && counter == 60) start(true);
    if(counter == 156) start(false);
    if(counter == 160) counter = 0;

    if(starting && (startCounter-- == 0)) {
      starting = false;
      pixelCounter = 8;
      widthCounter = width();
    }

    output = 0;
    if(pixelCounter) {
      stepPixelCounter();
      output = graphics[delay].bit(reflect ? (7 - pixelCounter) : pixelCounter);
    }
  }
}
