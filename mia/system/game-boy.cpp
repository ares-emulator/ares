struct GameBoy : System {
  auto name() -> string override { return "Game Boy"; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
};

auto GameBoy::load(string location) -> LoadResult {
  this->location = locate();
  pak = new vfs::directory;
  pak->append("boot.rom", Resource::GameBoy::BootDMG1);
  return successful;
}

auto GameBoy::save(string location) -> bool {
  return true;
}
