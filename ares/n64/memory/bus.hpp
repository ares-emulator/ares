template<u32 Size>
inline auto Bus::read(u32 address) -> u64 {
  static constexpr u64 unmapped = 0;
  address &= 0x1fff'ffff - (Size - 1);

  if(address <= 0x007f'ffff) return rdram.ram.read<Size>(address);
  if(address <= 0x03ef'ffff) return unmapped;
  if(address <= 0x03ff'ffff) return rdram.read<Size>(address);
  if(address <= 0x0407'ffff) return rsp.read<Size>(address);
  if(address <= 0x040f'ffff) return rsp.status.read<Size>(address);
  if(address <= 0x041f'ffff) return rdp.read<Size>(address);
  if(address <= 0x042f'ffff) return rdp.io.read<Size>(address);
  if(address <= 0x043f'ffff) return mi.read<Size>(address);
  if(address <= 0x044f'ffff) return vi.read<Size>(address);
  if(address <= 0x045f'ffff) return ai.read<Size>(address);
  if(address <= 0x046f'ffff) return pi.read<Size>(address);
  if(address <= 0x047f'ffff) return ri.read<Size>(address);
  if(address <= 0x048f'ffff) return si.read<Size>(address);
  if(address <= 0x04ff'ffff) return unmapped;
  if(address <= 0x1fbf'ffff) return pi.read<Size>(address);
  if(address <= 0x1fc0'07bf) {
    if(pi.io.romLockout) return unmapped;
    return pi.rom.read<Size>(address);
  }
  if(address <= 0x1fc0'07ff) return pi.ram.read<Size>(address);
  return unmapped;
}

template<u32 Size>
inline auto Bus::write(u32 address, u64 data) -> void {
  address &= 0x1fff'ffff - (Size - 1);
  if constexpr(Accuracy::CPU::Recompiler) {
    cpu.recompiler.invalidate(address + 0); if constexpr(Size == Dual)
    cpu.recompiler.invalidate(address + 4);
  }

  if(address <= 0x007f'ffff) return rdram.ram.write<Size>(address, data);
  if(address <= 0x03ef'ffff) return;
  if(address <= 0x03ff'ffff) return rdram.write<Size>(address, data);
  if(address <= 0x0407'ffff) return rsp.write<Size>(address, data);
  if(address <= 0x040f'ffff) return rsp.status.write<Size>(address, data);
  if(address <= 0x041f'ffff) return rdp.write<Size>(address, data);
  if(address <= 0x042f'ffff) return rdp.io.write<Size>(address, data);
  if(address <= 0x043f'ffff) return mi.write<Size>(address, data);
  if(address <= 0x044f'ffff) return vi.write<Size>(address, data);
  if(address <= 0x045f'ffff) return ai.write<Size>(address, data);
  if(address <= 0x046f'ffff) return pi.write<Size>(address, data);
  if(address <= 0x047f'ffff) return ri.write<Size>(address, data);
  if(address <= 0x048f'ffff) return si.write<Size>(address, data);
  if(address <= 0x04ff'ffff) return;
  if(address <= 0x1fbf'ffff) return pi.write<Size>(address, data);
  if(address <= 0x1fc0'07bf) {
    if(pi.io.romLockout) return;
    return pi.rom.write<Size>(address, data);
  }
  if(address <= 0x1fc0'07ff) return pi.ram.write<Size>(address, data);
  return;
}
