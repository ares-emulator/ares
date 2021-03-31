struct Saturn : System {
  auto name() -> string override { return "Saturn"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto Saturn::load(string location) -> bool {
  auto bios = Pak::read(location);
  if(!bios) return false;

  this->location = locate();
  pak = new vfs::directory;
  pak->append("bios.rom", bios);

  return true;
}

auto Saturn::save(string location) -> bool {
  return true;
}
