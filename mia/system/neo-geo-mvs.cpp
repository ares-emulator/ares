struct NeoGeoMVS : System {
  auto name() -> string override { return "Neo Geo MVS"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto NeoGeoMVS::load(string location) -> bool {
  this->location = locate();
  pak = new vfs::directory;
  return true;
}

auto NeoGeoMVS::save(string location) -> bool {
  return true;
}
