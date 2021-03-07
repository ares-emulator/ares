struct GameBoy : System {
  auto name() -> string override { return "Game Boy"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto GameBoy::load(string location) -> bool {
  this->location = locate();
  pak = new vfs::directory;
  pak->append("boot.rom", Resource::GameBoy::BootDMG1);
  return true;
}

auto GameBoy::save(string location) -> bool {
  return true;
}
