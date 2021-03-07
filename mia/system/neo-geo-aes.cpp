struct NeoGeoAES : System {
  auto name() -> string override { return "Neo Geo AES"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto NeoGeoAES::load(string location) -> bool {
  this->location = locate();
  pak = new vfs::directory;
  return true;
}

auto NeoGeoAES::save(string location) -> bool {
  return true;
}
