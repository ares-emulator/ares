auto TIA::Missile::reset() -> void {
  counter = -4;
}

auto TIA::Missile::start() -> void {
  startCounter = 4;
  starting = true;
}

auto TIA::Missile::width() -> u8 {
  const int missileSizes[4] = {1, 2, 4, 8};
  return missileSizes[size];
}

auto TIA::Missile::stepPixelCounter() -> void {
  if(--widthCounter == 0) {
    pixelCounter--;
    widthCounter = width();
  }
}

auto TIA::Missile::step(u8 clocks) -> void {
  if(!clocks) return;
  while(clocks--) {
    counter++;

    if(counter == 156) start();
    if(counter == 160) counter = 0;

    if(starting && (startCounter-- == 0)) {
      starting = false;
      pixelCounter = 1;
      widthCounter = width();
    }

    output = 0;
    if(pixelCounter) {
      stepPixelCounter();
      output = enable;
    }
  }
}


/*
auto TIA::runMissile(n8 x, n1 index) -> n1 {
  auto& missile = this->missile[index];
  auto& player = this->player[index];
  auto position = missile.position;

  if(missile.reset) {
    auto offset = 3;
    if(player.size == 5) offset = 6;
    if(player.size == 7) offset = 10;
    missile.position = player.position + offset;
    return 0;
  }

  if(!missile.enable) return 0;

  const int missileSizes[4] = {1, 2, 4, 8};

  // Handle player stretch capability
  auto width = missileSizes[missile.size];
  auto repeatWidth = width;
  if(player.size == 5) repeatWidth *= 2;
  if(player.size == 7) repeatWidth *= 4;

  // Handle repeat capability
  auto repeat = 1;
  if(player.size == 1 || player.size == 2 || player.size == 4) repeat = 2;
  if(player.size == 3 || player.size == 6)                     repeat = 3;

  auto spacing = 8;
  if(player.size == 2 || player.size == 6) spacing = 24;
  if(player.size == 4                    ) spacing = 56;

  for(int i = 0; i < repeat; i++) {
    if (x >= position && x < position + width) {
      return 1;
    }

    position = (position + (spacing + repeatWidth)) % 160;
  }

  return 0;
}
*/
