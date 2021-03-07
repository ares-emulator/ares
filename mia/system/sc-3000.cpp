struct SC3000 : System {
  auto name() -> string override { return "SC-3000"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto SC3000::load(string location) -> bool {
  this->location = locate();
  pak = new vfs::directory;
  return true;
}

auto SC3000::save(string location) -> bool {
  return true;
}
