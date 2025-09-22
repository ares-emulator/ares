struct Mega32X : System {
  auto name() -> string override { return "Mega 32X"; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
};

auto Mega32X::load(string location) -> LoadResult {
  this->location = locate();
  pak = std::make_shared<vfs::directory>();
  pak->append("tmss.rom", Resource::MegaDrive::TMSS);
  pak->append("vector.rom", Resource::Mega32X::Vector);
  pak->append("sh2.boot.mrom", Resource::Mega32X::SH2BootM);
  pak->append("sh2.boot.srom", Resource::Mega32X::SH2BootS);
  return successful;
}

auto Mega32X::save(string location) -> bool {
  return true;
}
