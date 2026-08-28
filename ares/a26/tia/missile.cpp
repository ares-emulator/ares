auto TIA::ObjectPipeline::Missile::reset(u8 counter, n1 hblank) -> void {
  this->counter = counter;
  if(!rendering) return;
  if(renderCounter < 0) {
    renderCounter = -4 + (counter - 157);
    return;
  }

  switch(width()) {
  case 8: renderCounter = counter - 157 + (renderCounter >= 4 ? 4 : 0); break;
  case 4: renderCounter = counter - 157; break;
  case 2:
    if(hblank) rendering = renderCounter > 1;
    else if(renderCounter == 0) renderCounter++;
    break;
  default:
    if(hblank) rendering = renderCounter > 0;
    break;
  }
}

auto TIA::ObjectPipeline::Missile::start(n2 copy) -> void {
  this->copy = copy;
  rendering = 1;
  renderCounter = -4;
}

auto TIA::ObjectPipeline::Missile::nusiz(n8 data) -> void {
  copies = data.bit(0, 2);
  size = data.bit(4, 5);
  if(rendering && renderCounter >= width()) rendering = 0;
}

auto TIA::ObjectPipeline::Missile::width() -> u8 {
  return 1 << size;
}

auto TIA::ObjectPipeline::Missile::latchOutput() -> void {
  output = rendering && renderCounter >= 0 && enable && !lockedToPlayer;
}

auto TIA::ObjectPipeline::Missile::clock(u8 clocks, n1 regularClock, u8 hcounter) -> void {
  while(clocks--) {
    auto visible = rendering && (renderCounter >= 0
      || (moving && regularClock && renderCounter == -1 && width() < 4 && ((hcounter + 1) & 3) == 3));
    output = visible && enable && !lockedToPlayer;

    if(auto copy = ObjectPipeline::decode(copies, counter); copy && !lockedToPlayer) {
      start(copy);
    } else if(rendering) {
      if(renderCounter == -1) {
        effectiveWidth = width();
        if(moving && regularClock) {
          switch((hcounter + 1) & 3) {
          case 2: effectiveWidth = 0; break;
          case 3:
            effectiveWidth = width() == 1 ? 2 : width();
            if(width() < 4) renderCounter++;
            break;
          }
        }
      }
      if(++renderCounter >= (moving ? effectiveWidth : width())) rendering = 0;
    }

    if(++counter == 160) counter = 0;
  }
}
