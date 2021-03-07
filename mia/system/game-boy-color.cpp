struct GameBoyColor : System {
  auto name() -> string override { return "Game Boy Color"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto GameBoyColor::load(string location) -> bool {
  this->location = locate();
  pak = new vfs::directory;
  pak->append("boot.rom", Resource::GameBoyColor::BootCGB0);
  return true;
}

auto GameBoyColor::save(string location) -> bool {
  return true;
}
