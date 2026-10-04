auto TIA::ObjectPipeline::Player::reset(u8 counter) -> void {
  this->counter = counter;
  if(rendering && renderCounter + 5 < 4) renderCounter = -5 + (counter - 157);
}

auto TIA::ObjectPipeline::Player::start(n2 copy) -> void {
  this->copy = copy;
  rendering = 1;
  sampleCounter = 0;
  renderCounter = -5;
}

auto TIA::ObjectPipeline::Player::width() -> u8 {
  switch(size) {
  case 5:  return 2;
  case 7:  return 4;
  default: return 1;
  }
}

auto TIA::ObjectPipeline::Player::setDivider(u8 divider) -> void {
  this->divider = divider;
  renderCounterTripPoint = divider == 1 ? 0 : 1;
}

auto TIA::ObjectPipeline::Player::nusiz(n3 size, n1 hblank) -> void {
  //Size control and graphics scan state are separate on TIA-1A sheet 3.
  //Follow Stella's standard per-clock phase transitions.
  auto oldSize = this->size;
  this->size = size;
  dividerPending = width();

  auto previousCounter = counter ? counter - 1 : 159;
  if(!rendering) {
    if(auto copy = ObjectPipeline::decode(size, previousCounter)) start(copy);
  } else if(size != oldSize && renderCounter + 5 < 2) {
    s32 decodeCounter = counter - renderCounter - 6;
    if(decodeCounter < 0) decodeCounter += 160;
    if(!ObjectPipeline::decode(size, decodeCounter)) rendering = 0;
  }

  if(dividerPending == divider) return;
  if(!rendering) {
    setDivider(dividerPending);
    return;
  }

  auto delta = renderCounter + 5;
  if(divider == 1) {
    if(hblank ? delta < 4 : delta < 3) setDivider(dividerPending);
    else dividerChangeCounter = hblank && delta >= 5 ? 0 : 1;
    return;
  }

  if(dividerPending == 1) {
    if(delta < (hblank ? 4 : 3)) {
      setDivider(dividerPending);
    } else if(delta < (hblank ? 6 : 5)) {
      setDivider(dividerPending);
      renderCounter--;
    } else {
      dividerChangeCounter = hblank ? 0 : 1;
    }
    return;
  }

  if(renderCounter < 1 || (hblank && renderCounter % divider == 1)) {
    setDivider(dividerPending);
  } else {
    dividerChangeCounter = divider - (renderCounter - 1) % divider;
  }
}

auto TIA::ObjectPipeline::Player::missileResetCounter() const -> u8 {
  auto offset = divider == 1 ? 5 : divider == 2 ? 8 : 12;
  s32 resetCounter = counter - offset;
  if(resetCounter < 0) resetCounter += 160;
  return resetCounter;
}

auto TIA::ObjectPipeline::Player::latchOutput() -> void {
  output = 0;
  if(!rendering || renderCounter < renderCounterTripPoint || sampleCounter > 7) return;
  output = graphics[delay].bit(reflect ? sampleCounter : 7 - sampleCounter);
}

auto TIA::ObjectPipeline::Player::clock(u8 clocks) -> n1 {
  n1 resetMissile = 0;
  while(clocks--) {
    latchOutput();

    if(auto copy = ObjectPipeline::decode(size, counter)) {
      start(copy);
    } else if(rendering) {
      renderCounter++;
      if(divider == 1) {
        if(renderCounter > 0) sampleCounter++;
        if(renderCounter >= 0 && dividerChangeCounter >= 0 && dividerChangeCounter-- == 0) {
          setDivider(dividerPending);
        }
      } else {
        if(renderCounter > 1 && ((renderCounter - 1) & (divider - 1)) == 0) sampleCounter++;
        if(renderCounter > 0 && dividerChangeCounter >= 0 && dividerChangeCounter-- == 0) {
          setDivider(dividerPending);
        }
      }
      if(sampleCounter > 7) rendering = 0;
    }

    if(++counter == 160) counter = 0;
    resetMissile |= rendering && sampleCounter == 4 && copy == 1;
  }
  return resetMissile;
}
