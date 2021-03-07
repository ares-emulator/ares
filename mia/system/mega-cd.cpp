struct MegaCD : System {
  auto name() -> string override { return "Mega CD"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto MegaCD::load(string location) -> bool {
  auto bios = Pak::read(location);
  if(!bios) return false;

  this->location = locate();
  pak = new vfs::directory;
  pak->append("tmss.rom", Resource::MegaDrive::TMSS);
  pak->append("bios.rom", bios);
  pak->append("backup.ram", 8_KiB);

  Pak::load("backup.ram", ".bram");

  return true;
}

auto MegaCD::save(string location) -> bool {
  Pak::save("backup.ram", ".bram");

  return true;
}
