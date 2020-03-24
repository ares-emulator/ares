auto Cartridge::MBC7::serialize(serializer& s) -> void {
  eeprom.serialize(s);
  s.integer(io.rom.bank);
  s.integer(io.ram.enable[0]);
  s.integer(io.ram.enable[1]);
  s.integer(io.accelerometer.x);
  s.integer(io.accelerometer.y);
}

auto Cartridge::MBC7::EEPROM::serialize(serializer& s) -> void {
  M93LCx6::serialize(s);
  s.boolean(select);
  s.boolean(clock);
}
