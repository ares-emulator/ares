struct Famicom : System {
  auto name() -> string override { return "Famicom"; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
};

auto Famicom::load(string location) -> LoadResult {
  this->location = locate();
  pak = new vfs::directory;
  return LoadResult(successful);
}

auto Famicom::save(string location) -> bool {
  return true;
}
