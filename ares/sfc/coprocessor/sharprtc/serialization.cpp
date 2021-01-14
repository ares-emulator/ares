auto SharpRTC::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s((uint&)state);
  s(index);

  s(second);
  s(minute);
  s(hour);
  s(day);
  s(month);
  s(year);
  s(weekday);
}
