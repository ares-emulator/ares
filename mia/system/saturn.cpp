struct Saturn : System {
  auto name() -> string override { return "Saturn"; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
};

auto Saturn::load(string location) -> LoadResult {
  auto bios = Pak::read(location);
  if(!bios) return LoadResult(romNotFound);

  this->location = locate();
  pak = new vfs::directory;
  pak->append("bios.rom", bios);

  return LoadResult(successful);
}

auto Saturn::save(string location) -> bool {
  return true;
}
