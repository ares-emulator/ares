auto Cartridge::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(array_span<uint8_t>{ram.data, ram.size});
  s(eeprom);
  s(array_span<uint8_t>{rtc.data, rtc.size});

  if(rtc.size) {
    s(rtc.command);
    s(rtc.index);
    s(rtc.alarm);
    s(rtc.alarmHour);
    s(rtc.alarmMinute);
  }

  s(r.romBank0);
  s(r.romBank1);
  s(r.romBank2);
  s(r.sramBank);
  s(r.gpoEnable);
  s(r.gpoData);
}
