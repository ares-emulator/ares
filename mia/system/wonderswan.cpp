struct WonderSwan : System {
  auto name() -> string override { return "WonderSwan"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto WonderSwan::load(string location) -> bool {
  this->location = locate();
  pak = new vfs::directory;
  pak->append("boot.rom", Resource::WonderSwan::Boot);
  pak->append("save.eeprom", 128);

  Pak::load("save.eeprom", ".eeprom");

  return true;
}

auto WonderSwan::save(string location) -> bool {
  Pak::save("save.eeprom", ".eeprom");

  return true;
}
