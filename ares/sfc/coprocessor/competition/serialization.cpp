auto Competition::serialize(serializer& s) -> void {
  Thread::serialize(s);
  s(status);
  s(select);
  s(timerActive);
  s(scoreActive);
  s(timerSecondsRemaining);
  s(scoreSecondsRemaining);
}
