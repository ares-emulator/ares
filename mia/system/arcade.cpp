struct Arcade : System {
  auto name() -> string override { return "Arcade"; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
};

auto Arcade::load(string location) -> LoadResult {
  this->location = locate();
  pak = std::make_shared<vfs::directory>();
  return successful;
}

auto Arcade::save(string location) -> bool {
  return true;
}
