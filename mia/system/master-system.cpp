struct MasterSystem : System {
  auto name() -> string override { return "Master System"; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
};

auto MasterSystem::load(string location) -> LoadResult {
  auto bios = Pak::read(location);  //optional

  this->location = locate();
  pak = new vfs::directory;
  if(bios) pak->append("bios.rom", bios);

  return successful;
}

auto MasterSystem::save(string location) -> bool {
  return true;
}
