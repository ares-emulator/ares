auto POKEY::serialize(serializer& s) -> void {
  Thread::serialize(s);
  clock.serialize(s);
  audio.serialize(s);
  irq.serialize(s);
  pots.serialize(s);
  status.serialize(s);
  keyboard.serialize(s);
  serial.serialize(s);
  s(control);
  s(previousInput);
  s(coupledOutput);
}

auto POKEY::Clock::serialize(serializer& s) -> void {
  s(prescaler64);
  s(prescaler15);
  s(polynomial4);
  s(polynomial5);
  s(polynomial9);
  s(polynomial17);
  s(polynomial4History);
  s(polynomial5History);
  s(polynomial9History);
  s(polynomial17History);
}

auto POKEY::Audio::serialize(serializer& s) -> void {
  for(auto& item : channel) {
    s(item.frequency);
    s(item.control);
    s(item.counter);
    s(item.eventDelay);
    s(item.reloadDelay);
    s(item.output);
    s(item.filterLatch);
    s(item.filterSample);
    s(item.filterDelay);
  }
  s(control);
  s(timersRunning);
  s(startDelay);
  s(twoToneResyncPipeline);
}

auto POKEY::IRQ::serialize(serializer& s) -> void {
  s(enable);
  s(statusValue);
  for(auto& age : enabledAge) s(age);
  for(auto& age : disabledAge) s(age);
}

auto POKEY::Pots::serialize(serializer& s) -> void {
  for(auto& item : value) s(item);
  for(auto& item : target) s(item);
  s(allValue);
  s(counter);
  s(scanning);
}

auto POKEY::Status::serialize(serializer& s) -> void {
  s(value);
}

auto POKEY::Keyboard::serialize(serializer& s) -> void {
  s(codeValue);
  s(scanCounter);
  s(compareValue);
  s(state);
  s(shiftLatch);
  s(controlLatch);
  s(breakLatch);
}

auto POKEY::Serial::serialize(serializer& s) -> void {
  s(inputValue);
  s(inputShifter);
  s(inputProgress);
  s(outputHoldingValue);
  s(outputHoldingFull);
  s(outputShifter);
  s(outputProgress);
  s(inputClockPhase);
  s(outputClockPhase);
  s(outputLine);
  s(twoToneOutput);
  s(asynchronousReceiving);
  s(previousInputLine);
  s(previousExternalClock);
}
