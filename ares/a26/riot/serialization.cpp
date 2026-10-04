auto RIOT::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(ram);

  s(timer.counter);
  s(timer.interval);
  s(timer.prescaler);
  s(timer.interruptEnable);
  s(timer.interruptFlag);
  s(timer.justWrapped);

  for(auto n : range(2)) {
    s(port[n].data);
    s(port[n].direction);
  }

  s(pa7.positiveEdge);
  s(pa7.interruptEnable);
  s(pa7.interruptFlag);
  s(pa7.level);

  s(leftDifficulty);
  s(leftDifficultyLatch);
  s(rightDifficulty);
  s(rightDifficultyLatch);
  s(tvType);
  s(tvTypeLatch);

  if(s.reading()) drivePortA();
}
