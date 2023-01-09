struct ZXSpectrum128 : System {
  auto name() -> string override { return "ZX Spectrum 128"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto ZXSpectrum128::load(string location) -> bool {
  this->location = locate();
  pak = new vfs::directory;
  pak->append("bios.rom", Resource::ZXSpectrum128::BIOS);
  pak->append("sub.rom", Resource::ZXSpectrum128::Sub);
  return true;
}

auto ZXSpectrum128::save(string location) -> bool {
  return true;
}
