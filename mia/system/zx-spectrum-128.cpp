struct ZXSpectrum128 : System {
  auto name() -> string override { return "ZX Spectrum 128"; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
};

auto ZXSpectrum128::load(string location) -> LoadResult {
  this->location = locate();
  pak = std::make_shared<vfs::directory>();
  pak->append("bios.rom", Resource::ZXSpectrum128::BIOS);
  pak->append("sub.rom", Resource::ZXSpectrum128::Sub);
  return successful;
}

auto ZXSpectrum128::save(string location) -> bool {
  return true;
}
