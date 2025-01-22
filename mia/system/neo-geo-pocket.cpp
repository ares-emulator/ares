struct NeoGeoPocket : System {
  auto name() -> string override { return "Neo Geo Pocket"; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
};

auto NeoGeoPocket::load(string location) -> LoadResult {
  auto bios = Pak::read(location);
  if(!bios) return romNotFound;

  this->location = locate();
  pak = new vfs::directory;
  pak->append("bios.rom", bios);
  pak->append("cpu.ram", 12_KiB);
  pak->append("apu.ram", 4_KiB);

  Pak::load("cpu.ram", ".cram");
  Pak::load("apu.ram", ".aram");

  return successful;
}

auto NeoGeoPocket::save(string location) -> bool {
  Pak::save("cpu.ram", ".cram");
  Pak::save("apu.ram", ".aram");

  return true;
}
