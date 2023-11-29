template<u32 Size>
inline auto Bus::read(u32 address, Thread& thread) -> u64 {
  static constexpr u64 unmapped = 0;
  static_assert(Size == Byte || Size == Half || Size == Word || Size == Dual);

  if(address <= 0x007f'ffff) return rdram.ram.read<Size>(address);
  if(address <= 0x03ef'ffff) return unmapped;
  if(address <= 0x03ff'ffff) return rdram.read<Size>(address, thread);
  if(address <= 0x0407'ffff) return rsp.read<Size>(address, thread);
  if(address <= 0x040f'ffff) return rsp.status.read<Size>(address, thread);
  if(address <= 0x041f'ffff) return rdp.read<Size>(address, thread);
  if(address <= 0x042f'ffff) return rdp.io.read<Size>(address, thread);
  if(address <= 0x043f'ffff) return mi.read<Size>(address, thread);
  if(address <= 0x044f'ffff) return vi.read<Size>(address, thread);
  if(address <= 0x045f'ffff) return ai.read<Size>(address, thread);
  if(address <= 0x046f'ffff) return pi.read<Size>(address, thread);
  if(address <= 0x047f'ffff) return ri.read<Size>(address, thread);
  if(address <= 0x048f'ffff) return si.read<Size>(address, thread);
  if(address <= 0x04ff'ffff) return unmapped;
  if(address <= 0x1fbf'ffff) return pi.read<Size>(address, thread);
  if(address <= 0x1fcf'ffff) return si.read<Size>(address, thread);
  if(address <= 0x7fff'ffff) return pi.read<Size>(address, thread);
  return unmapped;
}

template<u32 Size>
inline auto Bus::readBurst(u32 address, u32 *data, Thread& thread) -> void {
  static_assert(Size == DCache || Size == ICache);

  if(address <= 0x03ff'ffff) {
    data[0] = read<Word>(address | 0x0, thread);
    data[1] = read<Word>(address | 0x4, thread);
    data[2] = read<Word>(address | 0x8, thread);
    data[3] = read<Word>(address | 0xc, thread);
    if constexpr(Size == ICache) {
      data[4] = read<Word>(address | 0x10, thread);
      data[5] = read<Word>(address | 0x14, thread);
      data[6] = read<Word>(address | 0x18, thread);
      data[7] = read<Word>(address | 0x1c, thread);      
    }
    return;
  }

  debug(unusual, "[Bus::readBurst] CPU frozen because of cached read to non-RDRAM area: 0x", hex(address, 8L));
  cpu.scc.sysadFrozen = true;
}

template<u32 Size>
inline auto Bus::write(u32 address, u64 data, Thread& thread) -> void {
  static_assert(Size == Byte || Size == Half || Size == Word || Size == Dual);
  if constexpr(Accuracy::CPU::Recompiler) {
    cpu.recompiler.invalidate(address + 0); if constexpr(Size == Dual)
    cpu.recompiler.invalidate(address + 4);
  }

  if(address <= 0x007f'ffff) return rdram.ram.write<Size>(address, data);
  if(address <= 0x03ef'ffff) return;
  if(address <= 0x03ff'ffff) return rdram.write<Size>(address, data, thread);
  if(address <= 0x0407'ffff) return rsp.write<Size>(address, data, thread);
  if(address <= 0x040f'ffff) return rsp.status.write<Size>(address, data, thread);
  if(address <= 0x041f'ffff) return rdp.write<Size>(address, data, thread);
  if(address <= 0x042f'ffff) return rdp.io.write<Size>(address, data, thread);
  if(address <= 0x043f'ffff) return mi.write<Size>(address, data, thread);
  if(address <= 0x044f'ffff) return vi.write<Size>(address, data, thread);
  if(address <= 0x045f'ffff) return ai.write<Size>(address, data, thread);
  if(address <= 0x046f'ffff) return pi.write<Size>(address, data, thread);
  if(address <= 0x047f'ffff) return ri.write<Size>(address, data, thread);
  if(address <= 0x048f'ffff) return si.write<Size>(address, data, thread);
  if(address <= 0x04ff'ffff) return;
  if(address <= 0x1fbf'ffff) return pi.write<Size>(address, data, thread);
  if(address <= 0x1fcf'ffff) return si.write<Size>(address, data, thread);
  if(address <= 0x7fff'ffff) return pi.write<Size>(address, data, thread);
  return;
}

template<u32 Size>
inline auto Bus::writeBurst(u32 address, u32 *data, Thread& thread) -> void {
  static_assert(Size == DCache || Size == ICache);

  if(address <= 0x03ff'ffff) {
    write<Word>(address | 0x0, data[0], thread);
    write<Word>(address | 0x4, data[1], thread);
    write<Word>(address | 0x8, data[2], thread);
    write<Word>(address | 0xc, data[3], thread);
    if constexpr(Size == ICache) {
      write<Word>(address | 0x10, data[4], thread);
      write<Word>(address | 0x14, data[5], thread);
      write<Word>(address | 0x18, data[6], thread);
      write<Word>(address | 0x1c, data[7], thread);
    }
    return;
  }

  debug(unusual, "[Bus::readBurst] CPU frozen because of cached write to non-RDRAM area: 0x", hex(address, 8L));
  cpu.scc.sysadFrozen = true;
}
