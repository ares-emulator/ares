struct SC3000 : System {
  auto name() -> string override { return "SC-3000"; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
};

auto SC3000::load(string location) -> LoadResult {
  this->location = locate();
  pak = std::make_shared<vfs::directory>();
  return successful;
}

auto SC3000::save(string location) -> bool {
  return true;
}
