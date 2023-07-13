auto RTC::serialize(serializer& s) -> void {
  s(io.registerBank);
  s(io.registerIndex);
  s(io.timerEnable);
  s(sram);
  Thread::serialize(s);
}
