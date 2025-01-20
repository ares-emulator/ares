struct SuperGrafx : System {
  auto name() -> string override { return "SuperGrafx"; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
};

auto SuperGrafx::load(string location) -> LoadResult {
  this->location = locate();
  pak = new vfs::directory;
  pak->append("backup.ram", 2_KiB);

  Pak::load("backup.ram", ".bram");

  return LoadResult(successful);
}

auto SuperGrafx::save(string location) -> bool {
  Pak::save("backup.ram", ".bram");

  return true;
}
