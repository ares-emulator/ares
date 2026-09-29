auto Cartridge::serialize(serializer& s) -> void {
  s(ram);
  s(eeprom);
  s(eepromBusy);
  s(flash);
  s(isviewer.ram);
  s(rtc);

  if(sc64) {
    sc64->serialize(s);
  } else {
    SC64 stub{*this};
    stub.buffer.allocate(SC64::BlockRamSize, 0);
    stub.serialize(s);
  }
}
