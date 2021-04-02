struct WonderSwanColor : System {
  auto name() -> string override { return "WonderSwan Color"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto WonderSwanColor::load(string location) -> bool {
  this->location = locate();
  pak = new vfs::directory;
  pak->append("boot.rom", Resource::WonderSwanColor::Boot);
  pak->append("save.eeprom", 2048);

  Pak::load("save.eeprom", ".eeprom");

  return true;
}

auto WonderSwanColor::save(string location) -> bool {
  Pak::save("save.eeprom", ".eeprom");

  return true;
}
