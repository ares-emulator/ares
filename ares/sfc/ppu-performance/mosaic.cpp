inline auto PPU::Mosaic::enable() const -> bool {
  if(self.bg1.io.mosaicEnable) return true;
  if(self.bg2.io.mosaicEnable) return true;
  if(self.bg3.io.mosaicEnable) return true;
  if(self.bg4.io.mosaicEnable) return true;
  return false;
}

inline auto PPU::Mosaic::voffset() const -> u32 {
  return size - vcounter;
}

auto PPU::Mosaic::scanline() -> void {
  if(self.vcounter() == 1) {
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
