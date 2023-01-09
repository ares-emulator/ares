struct ZXSpectrum : System {
  auto name() -> string override { return "ZX Spectrum"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto ZXSpectrum::load(string location) -> bool {
  this->location = locate();
  pak = new vfs::directory;
  pak->append("bios.rom", Resource::ZXSpectrum::BIOS);
  return true;
}

auto ZXSpectrum::save(string location) -> bool {
  return true;
}
