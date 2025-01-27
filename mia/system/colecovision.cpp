struct ColecoVision : System {
  auto name() -> string override { return "ColecoVision"; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
};

auto ColecoVision::load(string location) -> LoadResult {
  auto bios = Pak::read(location);
  if(!bios) return romNotFound;

  this->location = locate();
  pak = new vfs::directory;
  pak->append("bios.rom", bios);
  return successful;
}

auto ColecoVision::save(string location) -> bool {
  return true;
}
