struct Famicom : System {
  auto name() -> string override { return "Famicom"; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
};

auto Famicom::load(string location) -> LoadResult {
  this->location = locate();
  pak = std::make_shared<vfs::directory>();
  return successful;
}

auto Famicom::save(string location) -> bool {
  return true;
}
