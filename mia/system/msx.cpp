struct MSX : System {
  auto name() -> string override { return "MSX"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto MSX::load(string location) -> bool {
  auto bios = Pak::read(location);
  if(!bios) return false;

  this->location = locate();
  pak = new vfs::directory;
  pak->append("bios.rom", bios);
  return true;
}

auto MSX::save(string location) -> bool {
  return true;
}
