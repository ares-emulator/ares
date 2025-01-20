struct ZXSpectrum : System {
  auto name() -> string override { return "ZX Spectrum"; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
};

auto ZXSpectrum::load(string location) -> LoadResult {
  this->location = locate();
  pak = new vfs::directory;
  pak->append("bios.rom", Resource::ZXSpectrum::BIOS);
  return LoadResult(successful);
}

auto ZXSpectrum::save(string location) -> bool {
  return true;
}
