struct MegaDrive : System {
  auto name() -> string override { return "Mega Drive"; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
};

auto MegaDrive::load(string location) -> LoadResult {
  this->location = locate();
  pak = new vfs::directory;
  pak->append("tmss.rom", Resource::MegaDrive::TMSS);
  return successful;
}

auto MegaDrive::save(string location) -> bool {
  return true;
}
