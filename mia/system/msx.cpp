struct MSX : System {
  auto name() -> string override { return "MSX"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto MSX::load(string location) -> bool {
  this->location = locate();
  pak = new vfs::directory;
  pak->append("bios.rom", Resource::MSX::BIOS);
  return true;
}

auto MSX::save(string location) -> bool {
  return true;
}
