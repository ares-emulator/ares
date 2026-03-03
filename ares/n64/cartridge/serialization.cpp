auto Cartridge::serialize(serializer& s) -> void {
  s(ram);
  s(eeprom);
  s(eepromBusy);
  s(flash);
  s(rtc);
}
