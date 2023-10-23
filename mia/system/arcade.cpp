struct Arcade : System {
  auto name() -> string override { return "Arcade"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto Arcade::load(string location) -> bool {
  this->location = locate();
  pak = new vfs::directory;
  return true;
}

auto Arcade::save(string location) -> bool {
  return true;
}
