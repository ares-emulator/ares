auto Cartridge::serialize(serializer& s) -> void {
  s(mrom);
  if(has.sram) s(sram);
  if(has.eeprom) s(eeprom);
  if(has.flash) s(flash);
}

auto Cartridge::MROM::serialize(serializer& s) -> void {
  s(size);
  s(mask);
}

auto Cartridge::SRAM::serialize(serializer& s) -> void {
  s(array_span<u8>{data, size});
  s(size);
  s(mask);
}

auto Cartridge::EEPROM::serialize(serializer& s) -> void {
  s(array_span<u8>{data, size});
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
  s(array_span<u8>{data, size});
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
