struct SG1000 : System {
  auto name() -> string override { return "SG-1000"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto SG1000::load(string location) -> bool {
  this->location = locate();
  pak = new vfs::directory;
  return true;
}

auto SG1000::save(string location) -> bool {
  return true;
}
