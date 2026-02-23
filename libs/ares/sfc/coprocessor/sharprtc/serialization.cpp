auto SharpRTC::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s((u32&)state);
  s(index);

  s(second);
  s(minute);
  s(hour);
  s(day);
  s(month);
  s(year);
  s(weekday);
}
