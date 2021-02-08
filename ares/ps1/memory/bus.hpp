inline auto Bus::mmio(u32 address) -> Memory::Interface& {
  address &= 0x1fff'ffff;
  if(address <= 0x1eff'ffff) return unmapped;
  if(address <= 0x1f7f'ffff) return expansion1;
  if(address <= 0x1f80'03ff) return cpu.scratchpad;
  if(address <= 0x1f80'0fff) return unmapped;
  if(address <= 0x1f80'103f) return memory;
  if(address <= 0x1f80'105f) return peripheral;
  if(address <= 0x1f80'106f) return memory;
  if(address <= 0x1f80'107f) return interrupt;
  if(address <= 0x1f80'10ff) return dma;
  if(address <= 0x1f80'112f) return timer;
  if(address <= 0x1f80'17ff) return unmapped;
  if(address <= 0x1f80'180f) return disc;
  if(address <= 0x1f80'181f) return gpu;
  if(address <= 0x1f80'182f) return mdec;
  if(address <= 0x1f80'1bff) return unmapped;
  if(address <= 0x1f80'1fff) return spu;
  if(address <= 0x1f80'2fff) return expansion2;
  if(address <= 0x1f9f'ffff) return unmapped;
  if(address <= 0x1fbf'ffff) return expansion3;
  return unmapped;
}

template<u32 Size>
inline auto Bus::read(u32 address) -> u32 {
  address &= 0x1fff'ffff;
  if(address <= 0x007f'ffff) return cpu.ram.read<Size>(address);
  if(address >= 0x1fc0'0000) return bios.read<Size>(address);
  return mmio(address).read<Size>(address);
}

template<u32 Size>
inline auto Bus::write(u32 address, u32 data) -> void {
  address &= 0x1fff'ffff;
  if(address <= 0x007f'ffff) {
    if constexpr(Accuracy::CPU::Recompiler) cpu.recompiler.invalidate(address);
    return cpu.ram.write<Size>(address, data);
  }
  if(address >= 0x1fc0'0000) return bios.write<Size>(address, data);
  return mmio(address).write<Size>(address, data);
}
