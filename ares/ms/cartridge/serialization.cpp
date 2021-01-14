auto Cartridge::serialize(serializer& s) -> void {
  s(ram);
  s(mapper.shift);
  s(mapper.ramPage2);
  s(mapper.ramEnablePage2);
  s(mapper.ramEnablePage3);
  s(mapper.romWriteEnable);
  s(mapper.romPage0);
  s(mapper.romPage1);
  s(mapper.romPage2);
}
