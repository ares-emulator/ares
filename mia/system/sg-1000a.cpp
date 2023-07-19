struct SG1000A : System {
  auto name() -> string override { return "SG-1000A"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto SG1000A::load(string location) -> bool {
  this->location = locate();
  pak = new vfs::directory;
  return true;
}

auto SG1000A::save(string location) -> bool {
  return true;
}
