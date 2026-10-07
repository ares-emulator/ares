//the Mega Duck has no boot ROM, so its system pak is empty: the cartridge is
//the entire machine as far as the core is concerned
struct MegaDuck : System {
  auto name() -> string override { return "Mega Duck"; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
};

auto MegaDuck::load(string location) -> LoadResult {
  this->location = locate();
  pak = std::make_shared<vfs::directory>();
  return successful;
}

auto MegaDuck::save(string location) -> bool {
  return true;
}
