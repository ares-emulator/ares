struct GameBoyAdvance : System {
  auto name() -> string override { return "Game Boy Advance"; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
};

auto GameBoyAdvance::load(string location) -> LoadResult {
  auto bios = Pak::read(location);
  if(!bios) return romNotFound;

  this->location = locate();
  pak = new vfs::directory;
  pak->append("bios.rom", bios);
  return successful;
}

auto GameBoyAdvance::save(string location) -> bool {
  return true;
}
