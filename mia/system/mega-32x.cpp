struct Mega32X : System {
  auto name() -> string override { return "Mega 32X"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto Mega32X::load(string location) -> bool {
  this->location = locate();
  pak = new vfs::directory;
  pak->append("tmss.rom", Resource::MegaDrive::TMSS);
  pak->append("vector.rom", Resource::Mega32X::Vector);
  pak->append("sh2.boot.mrom", Resource::Mega32X::SH2BootM);
  pak->append("sh2.boot.srom", Resource::Mega32X::SH2BootS);
  return true;
}

auto Mega32X::save(string location) -> bool {
  return true;
}
