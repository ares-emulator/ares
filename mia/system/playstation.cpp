struct PlayStation : System {
  auto name() -> string override { return "PlayStation"; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
};

auto PlayStation::load(string location) -> LoadResult {
  auto bios = Pak::read(location);
  if(bios.empty()) return romNotFound;

  this->location = locate();
  pak = new vfs::directory;
  pak->append("bios.rom", bios);
  return successful;
}

auto PlayStation::save(string location) -> bool {
  return true;
}
