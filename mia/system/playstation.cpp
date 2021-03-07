struct PlayStation : System {
  auto name() -> string override { return "PlayStation"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto PlayStation::load(string location) -> bool {
  auto bios = Pak::read(location);
  if(!bios) return false;

  this->location = locate();
  pak = new vfs::directory;
  pak->append("bios.rom", bios);

  return true;
}

auto PlayStation::save(string location) -> bool {
  return true;
}
