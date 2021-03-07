struct SuperGrafx : System {
  auto name() -> string override { return "SuperGrafx"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto SuperGrafx::load(string location) -> bool {
  this->location = locate();
  pak = new vfs::directory;
  pak->append("backup.ram", 2_KiB);

  Pak::load("backup.ram", ".bram");

  return true;
}

auto SuperGrafx::save(string location) -> bool {
  Pak::save("backup.ram", ".bram");

  return true;
}
