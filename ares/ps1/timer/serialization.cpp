auto Timer::serialize(serializer& s) -> void {
  s(counter.dotclock);
  s(counter.divclock);

  for(auto& t : timers) {
    s(t.counter);
    s(t.target);
    s(t.synchronize);
    s(t.mode);
    s(t.resetMode);
    s(t.irqOnTarget);
    s(t.irqOnSaturate);
    s(t.irqRepeat);
    s(t.irqMode);
    s(t.clock);
    s(t.divider);
    s(t.irqLine);
    s(t.reachedTarget);
    s(t.reachedSaturate);
    s(t.unknown);
    s(t.paused);
    s(t.irqTriggered);
  }
}
