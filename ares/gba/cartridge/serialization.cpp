auto Cartridge::serialize(serializer& s) -> void {
  s(mrom);
  if(has.sram) s(sram);
  if(has.eeprom) s(eeprom);
  if(has.flash) s(flash);
  if(has.rtc) s(gpio);
  if(has.rtc) s(rtc);
}

auto Cartridge::MROM::serialize(serializer& s) -> void {
  s(size);
  s(mask);
  s(pageAddr);
  s(burst);
}

auto Cartridge::SRAM::serialize(serializer& s) -> void {
  s(std::span<n8>{data, size});
  s(size);
  s(mask);
}

auto Cartridge::EEPROM::serialize(serializer& s) -> void {
  s(std::span<n8>{data, size});
  s(size);
  s(mask);
  s(test);
  s(bits);
  s((u32&)mode);
  s(offset);
  s(address);
  s(addressbits);
}

auto Cartridge::FLASH::serialize(serializer& s) -> void {
  s(std::span<n8>{data, size});
  s(size);
  s(id);
  s(unlockhi);
  s(unlocklo);
  s(idmode);
  s(erasemode);
  s(bankselect);
  s(writeselect);
  s(bank);
}

auto Cartridge::GPIO::serialize(serializer& s) -> void {
  s(latch);
  s(direction);
  s(readEnable);
}

auto Cartridge::RTC::serialize(serializer& s) -> void {
  S3511A::serialize(s);
  Thread::serialize(s);
  s(irq);
}
