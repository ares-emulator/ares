struct MegaCD32X : System {
  auto name() -> string override { return "Mega CD 32X"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto MegaCD32X::load(string location) -> bool {
  auto bios = Pak::read(location);
  if(!bios) return false;

  this->location = locate();
  pak = new vfs::directory;
  pak->append("tmss.rom", Resource::MegaDrive::TMSS);
  pak->append("bios.rom", bios);
  pak->append("vector.rom", Resource::Mega32X::Vector);
  pak->append("sh2.boot.mrom", Resource::Mega32X::SH2BootM);
  pak->append("sh2.boot.srom", Resource::Mega32X::SH2BootS);
  pak->append("backup.ram", 8_KiB);

  Pak::load("backup.ram", ".bram");

  return true;
}

auto MegaCD32X::save(string location) -> bool {
  Pak::save("backup.ram", ".bram");

  return true;
}
