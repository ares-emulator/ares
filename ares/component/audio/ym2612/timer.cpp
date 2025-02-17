auto YM2612::TimerA::run() -> void {
  if(!++counter)
    line |= irq & enableLatch;
  if(enableLatch < enable || !counter)
    counter = period;
  enableLatch = enable;
}

auto YM2612::TimerB::run() -> void {
  if(!++divider && !++counter)
    line |= irq & enableLatch;
  if(enableLatch < enable || !counter && !divider)
    counter = period; // do not reset divider on reenable
  enableLatch = enable;
}
