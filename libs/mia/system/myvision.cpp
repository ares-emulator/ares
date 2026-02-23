struct MyVision : System {
  auto name() -> string override { return "MyVision"; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
};

auto MyVision::load(string location) -> LoadResult {

  this->location = locate();
  pak = std::make_shared<vfs::directory>();

  return successful;
}

auto MyVision::save(string location) -> bool {
  return true;
}
