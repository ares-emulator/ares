auto Cartridge::serialize(serializer& s) -> void {
  if(ram) s.array(ram.data(), ram.size());

  s.integer(mapper.shift);
  s.integer(mapper.ramPage2);
  s.integer(mapper.ramEnablePage2);
  s.integer(mapper.ramEnablePage3);
  s.integer(mapper.romWriteEnable);
  s.integer(mapper.romPage0);
  s.integer(mapper.romPage1);
  s.integer(mapper.romPage2);
}
