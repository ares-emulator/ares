struct PCEngineLD : System {
  auto name() -> string override { return "PC Engine LD"; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
};

auto PCEngineLD::load(string location) -> LoadResult {
  this->location = locate();

  auto bios = Pak::read(location);
  if(bios.empty()) return romNotFound;

  pak = std::make_shared<vfs::directory>();
  pak->append("bios.rom", bios);
  pak->append("backup.ram", 2_KiB);

  Pak::load("backup.ram", ".bram");

  return successful;
}

auto PCEngineLD::save(string location) -> bool {
  Pak::save("backup.ram", ".bram");

  return true;
}
