struct Krokha : System {
  auto name() -> string override { return "Krokha"; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
};

auto Krokha::load(string location) -> LoadResult {

  this->location = locate();
  pak = std::make_shared<vfs::directory>();

  return successful;
}

auto Krokha::save(string location) -> bool {
  return true;
}
