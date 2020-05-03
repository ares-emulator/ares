inline auto Bus::readByte(u32 address) -> u8 {
  address &= 0x1fff'ffff;
  if(address <= 0x007f'ffff) return rdram.ram.readByte(address);
  if(address <= 0x03ef'ffff) return 0;
  if(address <= 0x03ff'ffff) return rdram.readIO(address);
  if(address <= 0x0400'0fff) return rsp.dmem.readByte(address);
  if(address <= 0x0400'1fff) return rsp.imem.readByte(address);
  if(address <= 0x040f'ffff) return rsp.readIO(address);
  if(address <= 0x041f'ffff) return rdp.readCommand(address);
  if(address <= 0x042f'ffff) return rdp.readSpan(address);
  if(address <= 0x043f'ffff) return mi.readIO(address);
  if(address <= 0x044f'ffff) return vi.readIO(address);
  if(address <= 0x045f'ffff) return ai.readIO(address);
  if(address <= 0x046f'ffff) return pi.readIO(address);
  if(address <= 0x047f'ffff) return ri.readIO(address);
  if(address <= 0x048f'ffff) return si.readIO(address);
  if(address <= 0x0fff'ffff) return 0;
  if(address <= 0x17ff'ffff) return cartridge.rom.readByte(address);
  return 0;
}

inline auto Bus::readHalf(u32 address) -> u16 {
  address &= 0x1fff'ffff;
  if(address <= 0x007f'ffff) return rdram.ram.readHalf(address);
  if(address <= 0x03ef'ffff) return 0;
  if(address <= 0x03ff'ffff) return rdram.readIO(address);
  if(address <= 0x0400'0fff) return rsp.dmem.readHalf(address);
  if(address <= 0x0400'1fff) return rsp.imem.readHalf(address);
  if(address <= 0x040f'ffff) return rsp.readIO(address);
  if(address <= 0x041f'ffff) return rdp.readCommand(address);
  if(address <= 0x042f'ffff) return rdp.readSpan(address);
  if(address <= 0x043f'ffff) return mi.readIO(address);
  if(address <= 0x044f'ffff) return vi.readIO(address);
  if(address <= 0x045f'ffff) return ai.readIO(address);
  if(address <= 0x046f'ffff) return pi.readIO(address);
  if(address <= 0x047f'ffff) return ri.readIO(address);
  if(address <= 0x048f'ffff) return si.readIO(address);
  if(address <= 0x0fff'ffff) return 0;
  if(address <= 0x17ff'ffff) return cartridge.rom.readHalf(address);
  return 0;
}

inline auto Bus::readWord(u32 address) -> u32 {
  address &= 0x1fff'ffff;
  if(address <= 0x007f'ffff) return rdram.ram.readWord(address);
  if(address <= 0x03ef'ffff) return 0;
  if(address <= 0x03ff'ffff) return rdram.readIO(address);
  if(address <= 0x0400'0fff) return rsp.dmem.readWord(address);
  if(address <= 0x0400'1fff) return rsp.imem.readWord(address);
  if(address <= 0x040f'ffff) return rsp.readIO(address);
  if(address <= 0x041f'ffff) return rdp.readCommand(address);
  if(address <= 0x042f'ffff) return rdp.readSpan(address);
  if(address <= 0x043f'ffff) return mi.readIO(address);
  if(address <= 0x044f'ffff) return vi.readIO(address);
  if(address <= 0x045f'ffff) return ai.readIO(address);
  if(address <= 0x046f'ffff) return pi.readIO(address);
  if(address <= 0x047f'ffff) return ri.readIO(address);
  if(address <= 0x048f'ffff) return si.readIO(address);
  if(address <= 0x0fff'ffff) return 0;
  if(address <= 0x17ff'ffff) return cartridge.rom.readWord(address);
  return 0;
}

inline auto Bus::readDouble(u32 address) -> u64 {
  address &= 0x1fff'ffff;
  if(address <= 0x07ff'ffff) return rdram.ram.readDouble(address);
  if(address <= 0x03ef'ffff) return 0;
  if(address <= 0x03ff'ffff) { u64 data = rdram.readIO(address) << 32; return data | rdram.readIO(address + 4); }
  if(address <= 0x0400'0fff) return rsp.dmem.readDouble(address);
  if(address <= 0x0400'1fff) return rsp.imem.readDouble(address);
  if(address <= 0x040f'ffff) { u64 data = rsp.readIO(address) << 32; return data | rsp.readIO(address + 4); }
  if(address <= 0x041f'ffff) { u64 data = rdp.readCommand(address) << 32; return data | rdp.readCommand(address + 4); }
  if(address <= 0x042f'ffff) { u64 data = rdp.readSpan(address) << 32; return data | rdp.readSpan(address + 4); }
  if(address <= 0x043f'ffff) { u64 data = mi.readIO(address) << 32; return data | mi.readIO(address + 4); }
  if(address <= 0x044f'ffff) { u64 data = vi.readIO(address) << 32; return data | vi.readIO(address + 4); }
  if(address <= 0x045f'ffff) { u64 data = ai.readIO(address) << 32; return data | ai.readIO(address + 4); }
  if(address <= 0x046f'ffff) { u64 data = pi.readIO(address) << 32; return data | pi.readIO(address + 4); }
  if(address <= 0x047f'ffff) { u64 data = ri.readIO(address) << 32; return data | ri.readIO(address + 4); }
  if(address <= 0x048f'ffff) { u64 data = si.readIO(address) << 32; return data | si.readIO(address + 4); }
  if(address <= 0x0fff'ffff) return 0;
  if(address <= 0x17ff'ffff) return cartridge.rom.readDouble(address);
  return 0;
}

inline auto Bus::writeByte(u32 address, u8 data) -> void {
  address &= 0x1fff'ffff;
  if(address <= 0x007f'ffff) return rdram.ram.writeByte(address, data);
  if(address <= 0x03ef'ffff) return;
  if(address <= 0x03ff'ffff) return rdram.writeIO(address, data);
  if(address <= 0x0400'0fff) return rsp.dmem.writeByte(address, data);
  if(address <= 0x0400'1fff) return rsp.imem.writeByte(address, data);
  if(address <= 0x040f'ffff) return rsp.writeIO(address, data);
  if(address <= 0x041f'ffff) return rdp.writeCommand(address, data);
  if(address <= 0x042f'ffff) return rdp.writeSpan(address, data);
  if(address <= 0x043f'ffff) return mi.writeIO(address, data);
  if(address <= 0x044f'ffff) return vi.writeIO(address, data);
  if(address <= 0x045f'ffff) return ai.writeIO(address, data);
  if(address <= 0x046f'ffff) return pi.writeIO(address, data);
  if(address <= 0x047f'ffff) return ri.writeIO(address, data);
  if(address <= 0x048f'ffff) return si.writeIO(address, data);
  if(address <= 0x0fff'ffff) return;
  if(address <= 0x17ff'ffff) return;
  return;
}

inline auto Bus::writeHalf(u32 address, u16 data) -> void {
  address &= 0x1fff'ffff;
  if(address <= 0x007f'ffff) return rdram.ram.writeHalf(address, data);
  if(address <= 0x03ef'ffff) return;
  if(address <= 0x03ff'ffff) return rdram.writeIO(address, data);
  if(address <= 0x0400'0fff) return rsp.dmem.writeHalf(address, data);
  if(address <= 0x0400'1fff) return rsp.imem.writeHalf(address, data);
  if(address <= 0x040f'ffff) return rsp.writeIO(address, data);
  if(address <= 0x041f'ffff) return rdp.writeCommand(address, data);
  if(address <= 0x042f'ffff) return rdp.writeSpan(address, data);
  if(address <= 0x043f'ffff) return mi.writeIO(address, data);
  if(address <= 0x044f'ffff) return vi.writeIO(address, data);
  if(address <= 0x045f'ffff) return ai.writeIO(address, data);
  if(address <= 0x046f'ffff) return pi.writeIO(address, data);
  if(address <= 0x047f'ffff) return ri.writeIO(address, data);
  if(address <= 0x048f'ffff) return si.writeIO(address, data);
  if(address <= 0x0fff'ffff) return;
  if(address <= 0x17ff'ffff) return;
  return;
}

inline auto Bus::writeWord(u32 address, u32 data) -> void {
  address &= 0x1fff'ffff;
  if(address <= 0x007f'ffff) return rdram.ram.writeWord(address, data);
  if(address <= 0x03ef'ffff) return;
  if(address <= 0x03ff'ffff) return rdram.writeIO(address, data);
  if(address <= 0x0400'0fff) return rsp.dmem.writeWord(address, data);
  if(address <= 0x0400'1fff) return rsp.imem.writeWord(address, data);
  if(address <= 0x040f'ffff) return rsp.writeIO(address, data);
  if(address <= 0x041f'ffff) return rdp.writeCommand(address, data);
  if(address <= 0x042f'ffff) return rdp.writeSpan(address, data);
  if(address <= 0x043f'ffff) return mi.writeIO(address, data);
  if(address <= 0x044f'ffff) return vi.writeIO(address, data);
  if(address <= 0x045f'ffff) return ai.writeIO(address, data);
  if(address <= 0x046f'ffff) return pi.writeIO(address, data);
  if(address <= 0x047f'ffff) return ri.writeIO(address, data);
  if(address <= 0x048f'ffff) return si.writeIO(address, data);
  if(address <= 0x0fff'ffff) return;
  if(address <= 0x17ff'ffff) return;
  return;
}

inline auto Bus::writeDouble(u32 address, u64 data) -> void {
  address &= 0x1fff'ffff;
  if(address <= 0x007f'ffff) return rdram.ram.writeDouble(address, data);
  if(address <= 0x03ef'ffff) return;
  if(address <= 0x03ff'ffff) { rdram.writeIO(address, data >> 32); return rdram.writeIO(address + 4, data); }
  if(address <= 0x0400'0fff) return rsp.dmem.writeDouble(address, data);
  if(address <= 0x0400'1fff) return rsp.imem.writeDouble(address, data);
  if(address <= 0x040f'ffff) { rsp.writeIO(address, data >> 32); return rsp.writeIO(address + 4, data); }
  if(address <= 0x041f'ffff) { rdp.writeCommand(address, data >> 32); return rdp.writeCommand(address + 4, data); }
  if(address <= 0x042f'ffff) { rdp.writeSpan(address, data >> 32); return rdp.writeSpan(address + 4, data); }
  if(address <= 0x043f'ffff) { mi.writeIO(address, data >> 32); return mi.writeIO(address + 4, data); }
  if(address <= 0x044f'ffff) { vi.writeIO(address, data >> 32); return vi.writeIO(address + 4, data); }
  if(address <= 0x045f'ffff) { ai.writeIO(address, data >> 32); return ai.writeIO(address + 4, data); }
  if(address <= 0x046f'ffff) { pi.writeIO(address, data >> 32); return pi.writeIO(address + 4, data); }
  if(address <= 0x047f'ffff) { ri.writeIO(address, data >> 32); return ri.writeIO(address + 4, data); }
  if(address <= 0x048f'ffff) { si.writeIO(address, data >> 32); return si.writeIO(address + 4, data); }
  if(address <= 0x0fff'ffff) return;
  if(address <= 0x17ff'ffff) return;
  return;
}
