struct MegaDrive : System {
  auto name() -> string override { return "Mega Drive"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto MegaDrive::load(string location) -> bool {
  this->location = locate();
  pak = new vfs::directory;
  pak->append("tmss.rom", Resource::MegaDrive::TMSS);
  return true;
}

auto MegaDrive::save(string location) -> bool {
  return true;
}
