//serialization.cpp
auto RIOT::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(ram);
  s(timer.counter);
  s(timer.interval);
  s(timer.reload);
  s(timer.interruptEnable);
  s(timer.interruptFlag);

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
