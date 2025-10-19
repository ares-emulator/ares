struct GameBoyColor : System {
  auto name() -> string override { return "Game Boy Color"; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
};

auto GameBoyColor::load(string location) -> LoadResult {
  this->location = locate();
  pak = std::make_shared<vfs::directory>();
  pak->append("boot.rom", Resource::GameBoyColor::BootCGB0);
  return successful;
}

auto GameBoyColor::save(string location) -> bool {
  return true;
}
