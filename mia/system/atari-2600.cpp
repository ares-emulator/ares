struct Atari2600 : System {
  auto name() -> string override { return "Atari 2600"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto Atari2600::load(string location) -> bool {
  this->location = locate();
  pak = new vfs::directory;
  return true;
}

auto Atari2600::save(string location) -> bool {
  return true;
}
