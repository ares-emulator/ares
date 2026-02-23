struct Atari2600 : System {
  auto name() -> string override { return "Atari 2600"; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
};

auto Atari2600::load(string location) -> LoadResult {
  this->location = locate();
  pak = std::make_shared<vfs::directory>();
  return successful;
}

auto Atari2600::save(string location) -> bool {
  return true;
}
