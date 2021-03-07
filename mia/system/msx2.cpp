struct MSX2 : System {
  auto name() -> string override { return "MSX2"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto MSX2::load(string location) -> bool {
  this->location = locate();
  pak = new vfs::directory;
  pak->append("bios.rom", Resource::MSX2::BIOS);
  pak->append("sub.rom", Resource::MSX2::Sub);
  return true;
}

auto MSX2::save(string location) -> bool {
  return true;
}
