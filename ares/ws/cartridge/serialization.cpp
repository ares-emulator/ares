auto Cartridge::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(ram);
  s(eeprom);
  s(rtc.ram);

  s(rtc.command);
  s(rtc.index);
  s(rtc.alarm);
  s(rtc.alarmHour);
  s(rtc.alarmMinute);

  s(io.romBank0);
  s(io.romBank1);
  s(io.romBank2);
  s(io.sramBank);
  s(io.gpoEnable);
  s(io.gpoData);
}
