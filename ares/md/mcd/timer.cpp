auto MCD::Timer::clock() -> void {
  if(counter && !--counter) {
    irq.raise();
  }
}

auto MCD::Timer::power(bool reset) -> void {
  irq = {};
  counter = 0;
}
