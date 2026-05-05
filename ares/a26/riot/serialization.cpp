auto RIOT::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(ram);

  s(timer.counter);
  s(timer.interval);
  s(timer.prescaler);
  s(timer.interruptEnable);
  s(timer.interruptFlag);
  s(timer.underflow);
  s(timer.holdZero);
  s(timer.justWrapped);

  for(auto n : range(2)) {
    s(port[n].data);
    s(port[n].direction);
  }

  s(leftDifficulty);
  s(leftDifficultyLatch);
  s(rightDifficulty);
  s(rightDifficultyLatch);
  s(tvType);
  s(tvTypeLatch);
}
