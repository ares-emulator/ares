auto Cartridge::serialize(serializer& s) -> void {
  s(ram);
  s(eeprom);
  s(eepromBusy);
  s(flash);
  s(isviewer.ram);
  s(rtc);
  // The serializer is positional, and SC64 presence is a frontend setting
  // rather than a property of the game: serialize a stub when no SC64 is
  // attached so the savestate layout stays fixed either way.
  if(sc64) {
    sc64->serialize(s);
  } else {
    SC64 stub{*this};
    stub.buffer.allocate(SC64::BlockRamSize, 0);
    stub.serialize(s);
  }
}
