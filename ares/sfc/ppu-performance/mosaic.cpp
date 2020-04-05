inline auto PPU::Mosaic::enable() const -> bool {
  if(ppu.bg1.io.mosaicEnable) return true;
  if(ppu.bg2.io.mosaicEnable) return true;
  if(ppu.bg3.io.mosaicEnable) return true;
  if(ppu.bg4.io.mosaicEnable) return true;
  return false;
}

inline auto PPU::Mosaic::voffset() const -> uint {
  return size - vcounter;
}

auto PPU::Mosaic::scanline() -> void {
  if(ppu.vcounter() == 1) {
    vcounter = enable() ? size + 1 : 0;
  }
  if(vcounter && !--vcounter) {
    vcounter = enable() ? size + 0 : 0;
  }
}

auto PPU::Mosaic::power() -> void {
  size = 1;
  vcounter = 0;
}
