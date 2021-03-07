struct GameBoyAdvance : System {
  auto name() -> string override { return "Game Boy Advance"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto GameBoyAdvance::load(string location) -> bool {
  auto bios = Pak::read(location);
  if(!bios) return false;

  this->location = locate();
  pak = new vfs::directory;
  pak->append("bios.rom", bios);
  return true;
}

auto GameBoyAdvance::save(string location) -> bool {
  return true;
}
