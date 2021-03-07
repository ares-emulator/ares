struct ColecoVision : System {
  auto name() -> string override { return "ColecoVision"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto ColecoVision::load(string location) -> bool {
  auto bios = Pak::read(location);
  if(!bios) return false;

  this->location = locate();
  pak = new vfs::directory;
  pak->append("bios.rom", bios);
  return true;
}

auto ColecoVision::save(string location) -> bool {
  return true;
}
