struct Famicom : System {
  auto name() -> string override { return "Famicom"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto Famicom::load(string location) -> bool {
  this->location = locate();
  pak = new vfs::directory;
  return true;
}

auto Famicom::save(string location) -> bool {
  return true;
}
