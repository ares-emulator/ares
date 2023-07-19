struct MyVision : System {
  auto name() -> string override { return "MyVision"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto MyVision::load(string location) -> bool {

  this->location = locate();
  pak = new vfs::directory;

  return true;
}

auto MyVision::save(string location) -> bool {
  return true;
}
