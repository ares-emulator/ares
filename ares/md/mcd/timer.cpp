auto MCD::Timer::clock() -> void {
  if(frequency && !--counter) {
    counter = frequency;
    irq.raise();
  }
}

auto MCD::Timer::power(bool reset) -> void {
  irq = {};
  frequency = 0;
  counter = 0;
}
