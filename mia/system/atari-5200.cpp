struct Atari5200 : System {
  auto name() -> string override { return "Atari 5200"; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
};

auto Atari5200::load(string location) -> LoadResult {
  auto bios = Pak::read(location);
  if(bios.empty()) return romNotFound;
  if(bios.size() != 2_KiB) return invalidROM;

  this->location = locate();
  pak = std::make_shared<vfs::directory>();
  pak->append("bios.rom", bios);
  return successful;
}

auto Atari5200::save(string location) -> bool {
  return true;
}
