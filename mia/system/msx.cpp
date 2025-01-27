struct MSX : System {
  auto name() -> string override { return "MSX"; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
};

auto MSX::load(string location) -> LoadResult {
  auto bios = Pak::read(location);
  if(!bios) return romNotFound;

  this->location = locate();
  pak = new vfs::directory;
  pak->append("bios.rom", bios);
  return successful;
}

auto MSX::save(string location) -> bool {
  return true;
}
