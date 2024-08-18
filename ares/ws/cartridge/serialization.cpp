auto Cartridge::serialize(serializer& s) -> void {
  if(has.sram) {
    s(ram);
  }

  if(has.eeprom) {
    s(eeprom);
  }

  if(has.rtc) {
    rtc.serialize(s);
  }

  s(io.romBank0);
  s(io.romBank1);
  s(io.romBank2);
  s(io.sramBank);
  s(io.gpoEnable);
  s(io.gpoData);
  s(io.flashEnable);

  if(has.flash) {
    s(rom);
    s(flash.unlock);
    s(flash.idmode);
    s(flash.programmode);
    s(flash.fastmode);
    s(flash.erasemode);
  }

  if(has.karnak) {
    karnak.serialize(s);
  }

  s(openbus);
}

auto Cartridge::RTC::serialize(serializer& s) -> void {
  Thread::serialize(s);
  s(ram);

  s(command);
  s(active);
  s(index);
  s(fetchedData);
  s(counter);
}

auto Cartridge::KARNAK::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(timerEnable);
  s(timerPeriod);
  s(timerCounter);
}
