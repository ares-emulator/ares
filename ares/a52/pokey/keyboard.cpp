auto POKEY::Keyboard::power() -> void {
  codeValue = 0xff;
  scanCounter = 0;
  compareValue = 0;
  state = State::Released;
  shiftLatch = 0;
  controlLatch = 0;
  breakLatch = 0;
}

auto POKEY::Keyboard::disable() -> void {
  self.status.setKeyboardDown(false);
  scanCounter = 0;
  compareValue = 0;
  state = State::Released;
}

auto POKEY::Keyboard::clock(bool debounce) -> void {
  scanCounter++;
  auto lines = self.sampleKeyboard(scanCounter);

  if(scanCounter == 0x00) {
    shiftLatch = lines.kr2;
    self.status.setShift(shiftLatch);
  }
  if(scanCounter == 0x10) controlLatch = lines.kr2;
  if(scanCounter == 0x30) {
    if(lines.kr2 && !breakLatch) self.irq.request(IRQ::Break);
    breakLatch = lines.kr2;
  }

  auto matches = !debounce || scanCounter == compareValue;
  switch(state) {
  case State::Released:
    if(lines.kr1) {
      compareValue = scanCounter;
      state = State::DebouncePress;
    }
    return;

  case State::DebouncePress:
    if(!matches) {
      if(lines.kr1) state = State::Released;
      return;
    }
    if(lines.kr1) {
      if(self.irq.pending(IRQ::Keyboard)) self.status.setKeyboardOverrun();
      codeValue = debounce ? compareValue : scanCounter;
      codeValue.bit(6) = shiftLatch;
      codeValue.bit(7) = controlLatch;
      self.status.setKeyboardDown(true);
      self.irq.request(IRQ::Keyboard);
      state = State::Pressed;
    } else {
      state = State::Released;
    }
    return;

  case State::Pressed:
    if(matches && !lines.kr1) state = State::DebounceRelease;
    return;

  case State::DebounceRelease:
    if(!matches) return;
    if(lines.kr1) {
      state = State::Pressed;
    } else {
      self.status.setKeyboardDown(false);
      state = State::Released;
    }
    return;
  }
}

auto POKEY::Keyboard::code() const -> n8 {
  return codeValue;
}
