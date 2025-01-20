struct WonderSwan : System {
  auto name() -> string override { return "WonderSwan"; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
};

auto WonderSwan::load(string location) -> LoadResult {
  this->location = locate();
  pak = new vfs::directory;
  pak->append("boot.rom", Resource::WonderSwan::Boot);
  pak->append("save.eeprom", 128);

  Pak::load("save.eeprom", ".eeprom");

  return LoadResult(successful);
}

auto WonderSwan::save(string location) -> bool {
  Pak::save("save.eeprom", ".eeprom");

  return true;
}
