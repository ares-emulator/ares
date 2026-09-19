//the 4KB boot ROM lives inside the system ASIC and is not redistributable, so
//it has to be supplied. two revisions exist and either will do: the common UMC
//one (crc32 07090415) and the later BIT one (crc32 03a5f3a7)
struct Gamate : System {
  auto name() -> string override { return "Gamate"; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
};

auto Gamate::load(string location) -> LoadResult {
  auto bios = Pak::read(location);
  if(bios.empty()) return romNotFound;

  this->location = locate();
  pak = std::make_shared<vfs::directory>();
  pak->append("bios.rom", bios);
  return successful;
}

auto Gamate::save(string location) -> bool {
  return true;
}
