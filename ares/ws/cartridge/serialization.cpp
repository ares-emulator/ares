auto Cartridge::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(ram);
  s(eeprom);
  s(rtc);
  if(rtc) {
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
