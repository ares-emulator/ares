struct GameGear : System {
  auto name() -> string override { return "Game Gear"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto GameGear::load(string location) -> bool {
  this->location = locate();
  pak = new vfs::directory;
  return true;
}

auto GameGear::save(string location) -> bool {
  return true;
}
