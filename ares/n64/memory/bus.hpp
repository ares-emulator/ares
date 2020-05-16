inline auto Bus::readByte(u32 address) -> u8 {
  static auto transform = [&](u32 data) -> u8 {
    switch(address & 3) {
    case 0: return data >> 24;
    case 1: return data >> 16;
    case 2: return data >>  8;
    case 3: return data >>  0;
    } unreachable;
  };

  address &= 0x1fff'ffff;
  if(address <= 0x007f'ffff) return rdram.ram.readByte(address);
  if(address <= 0x03ef'ffff) return 0;
  if(address <= 0x03ff'ffff) return transform(rdram.readIO(address));
  if(address <= 0x0400'0fff) return rsp.dmem.readByte(address);
  if(address <= 0x0400'1fff) return rsp.imem.readByte(address);
  if(address <= 0x0403'ffff) return 0;
  if(address <= 0x0407'ffff) return transform(rsp.readSCC(address));
  if(address <= 0x040f'ffff) return transform(rsp.readIO(address));
  if(address <= 0x041f'ffff) return transform(rdp.readSCC(address));
  if(address <= 0x042f'ffff) return transform(rdp.readIO(address));
  if(address <= 0x043f'ffff) return transform(mi.readIO(address));
  if(address <= 0x044f'ffff) return transform(vi.readIO(address));
  if(address <= 0x045f'ffff) return transform(ai.readIO(address));
  if(address <= 0x046f'ffff) return transform(pi.readIO(address));
  if(address <= 0x047f'ffff) return transform(ri.readIO(address));
  if(address <= 0x048f'ffff) return transform(si.readIO(address));
  if(address <= 0x0fff'ffff) return 0;
  if(address <= 0x1fbf'ffff) return cartridge.rom.readByte(address);
  if(address <= 0x1fc0'07bf) return pi.rom.readByte(address);
  if(address <= 0x1fc0'07ff) return pi.ram.readByte(address);
  return 0;
}

inline auto Bus::readHalf(u32 address) -> u16 {
  static auto transform = [&](u32 data) -> u16 {
    switch(address & 2) {
    case 0: return data >> 16;
    case 2: return data >>  0;
    } unreachable;
  };

  address &= 0x1fff'fffe;
  if(address <= 0x007f'ffff) return rdram.ram.readHalf(address);
  if(address <= 0x03ef'ffff) return 0;
  if(address <= 0x03ff'ffff) return transform(rdram.readIO(address));
  if(address <= 0x0400'0fff) return rsp.dmem.readHalf(address);
  if(address <= 0x0400'1fff) return rsp.imem.readHalf(address);
  if(address <= 0x0403'ffff) return 0;
  if(address <= 0x0407'ffff) return transform(rsp.readSCC(address));
  if(address <= 0x040f'ffff) return transform(rsp.readIO(address));
  if(address <= 0x041f'ffff) return transform(rdp.readSCC(address));
  if(address <= 0x042f'ffff) return transform(rdp.readIO(address));
  if(address <= 0x043f'ffff) return transform(mi.readIO(address));
  if(address <= 0x044f'ffff) return transform(vi.readIO(address));
  if(address <= 0x045f'ffff) return transform(ai.readIO(address));
  if(address <= 0x046f'ffff) return transform(pi.readIO(address));
  if(address <= 0x047f'ffff) return transform(ri.readIO(address));
  if(address <= 0x048f'ffff) return transform(si.readIO(address));
  if(address <= 0x0fff'ffff) return 0;
  if(address <= 0x1fbf'ffff) return cartridge.rom.readHalf(address);
  if(address <= 0x1fc0'07bf) return pi.rom.readHalf(address);
  if(address <= 0x1fc0'07ff) return pi.ram.readHalf(address);
  return 0;
}

inline auto Bus::readWord(u32 address) -> u32 {
  address &= 0x1fff'fffc;
  if(address <= 0x007f'ffff) return rdram.ram.readWord(address);
  if(address <= 0x03ef'ffff) return 0;
  if(address <= 0x03ff'ffff) return rdram.readIO(address);
  if(address <= 0x0400'0fff) return rsp.dmem.readWord(address);
  if(address <= 0x0400'1fff) return rsp.imem.readWord(address);
  if(address <= 0x0403'ffff) return 0;
  if(address <= 0x0407'ffff) return rsp.readSCC(address);
  if(address <= 0x040f'ffff) return rsp.readIO(address);
  if(address <= 0x041f'ffff) return rdp.readSCC(address);
  if(address <= 0x042f'ffff) return rdp.readIO(address);
  if(address <= 0x043f'ffff) return mi.readIO(address);
  if(address <= 0x044f'ffff) return vi.readIO(address);
  if(address <= 0x045f'ffff) return ai.readIO(address);
  if(address <= 0x046f'ffff) return pi.readIO(address);
  if(address <= 0x047f'ffff) return ri.readIO(address);
  if(address <= 0x048f'ffff) return si.readIO(address);
  if(address <= 0x0fff'ffff) return 0;
  if(address <= 0x1fbf'ffff) return cartridge.rom.readWord(address);
  if(address <= 0x1fc0'07bf) return pi.rom.readWord(address);
  if(address <= 0x1fc0'07ff) return pi.ram.readWord(address);
  return 0;
}

inline auto Bus::readDouble(u32 address) -> u64 {
  address &= 0x1fff'fff8;
  if(address <= 0x007f'ffff) return rdram.ram.readDouble(address);
  if(address <= 0x03ef'ffff) return 0;
  if(address <= 0x03ff'ffff) { u64 data = rdram.readIO(address); return data << 32 | rdram.readIO(address + 4); }
  if(address <= 0x0400'0fff) return rsp.dmem.readDouble(address);
  if(address <= 0x0400'1fff) return rsp.imem.readDouble(address);
  if(address <= 0x0403'ffff) return 0;
  if(address <= 0x0407'ffff) { u64 data = rsp.readSCC(address);  return data << 32 | rsp.readSCC (address + 4); }
  if(address <= 0x040f'ffff) { u64 data = rsp.readIO(address);   return data << 32 | rsp.readIO  (address + 4); }
  if(address <= 0x041f'ffff) { u64 data = rdp.readSCC(address);  return data << 32 | rdp.readSCC (address + 4); }
  if(address <= 0x042f'ffff) { u64 data = rdp.readIO(address);   return data << 32 | rdp.readIO  (address + 4); }
  if(address <= 0x043f'ffff) { u64 data = mi.readIO(address);    return data << 32 | mi.readIO   (address + 4); }
  if(address <= 0x044f'ffff) { u64 data = vi.readIO(address);    return data << 32 | vi.readIO   (address + 4); }
  if(address <= 0x045f'ffff) { u64 data = ai.readIO(address);    return data << 32 | ai.readIO   (address + 4); }
  if(address <= 0x046f'ffff) { u64 data = pi.readIO(address);    return data << 32 | pi.readIO   (address + 4); }
  if(address <= 0x047f'ffff) { u64 data = ri.readIO(address);    return data << 32 | ri.readIO   (address + 4); }
  if(address <= 0x048f'ffff) { u64 data = si.readIO(address);    return data << 32 | si.readIO   (address + 4); }
  if(address <= 0x0fff'ffff) return 0;
  if(address <= 0x1fbf'ffff) return cartridge.rom.readDouble(address);
  if(address <= 0x1fc0'07bf) return pi.rom.readDouble(address);
  if(address <= 0x1fc0'07ff) return pi.ram.readDouble(address);
  return 0;
}

inline auto Bus::writeByte(u32 address, u8 data) -> void {
  static auto transform = [&](u32 data) -> u32 {
    switch(address & 3) {
    case 0: return data << 24;
    case 1: return data << 16;
    case 2: return data <<  8;
    case 3: return data <<  0;
    } unreachable;
  };

  address &= 0x1fff'ffff;
  if(address <= 0x007f'ffff) return rdram.ram.writeByte(address, data);
  if(address <= 0x03ef'ffff) return;
  if(address <= 0x03ff'ffff) return rdram.writeIO(address, transform(data));
  if(address <= 0x0400'0fff) return rsp.dmem.writeByte(address, data);
  if(address <= 0x0400'1fff) return rsp.imem.writeByte(address, data);
  if(address <= 0x0403'ffff) return;
  if(address <= 0x0407'ffff) return rsp.writeSCC(address, transform(data));
  if(address <= 0x040f'ffff) return rsp.writeIO(address, transform(data));
  if(address <= 0x041f'ffff) return rdp.writeSCC(address, transform(data));
  if(address <= 0x042f'ffff) return rdp.writeIO(address, transform(data));
  if(address <= 0x043f'ffff) return mi.writeIO(address, transform(data));
  if(address <= 0x044f'ffff) return vi.writeIO(address, transform(data));
  if(address <= 0x045f'ffff) return ai.writeIO(address, transform(data));
  if(address <= 0x046f'ffff) return pi.writeIO(address, transform(data));
  if(address <= 0x047f'ffff) return ri.writeIO(address, transform(data));
  if(address <= 0x048f'ffff) return si.writeIO(address, transform(data));
  if(address <= 0x0fff'ffff) return;
  if(address <= 0x1fbf'ffff) return;
  if(address <= 0x1fc0'07bf) return;
  if(address <= 0x1fc0'07ff) return pi.ram.writeByte(address, data);
  return;
}

inline auto Bus::writeHalf(u32 address, u16 data) -> void {
  static auto transform = [&](u32 data) -> u32 {
    switch(address & 2) {
    case 0: return data << 16;
    case 2: return data <<  0;
    } unreachable;
  };

  address &= 0x1fff'fffe;
  if(address <= 0x007f'ffff) return rdram.ram.writeHalf(address, data);
  if(address <= 0x03ef'ffff) return;
  if(address <= 0x03ff'ffff) return rdram.writeIO(address, transform(data));
  if(address <= 0x0400'0fff) return rsp.dmem.writeHalf(address, data);
  if(address <= 0x0400'1fff) return rsp.imem.writeHalf(address, data);
  if(address <= 0x0403'ffff) return;
  if(address <= 0x0407'ffff) return rsp.writeSCC(address, transform(data));
  if(address <= 0x040f'ffff) return rsp.writeIO(address, transform(data));
  if(address <= 0x041f'ffff) return rdp.writeSCC(address, transform(data));
  if(address <= 0x042f'ffff) return rdp.writeIO(address, transform(data));
  if(address <= 0x043f'ffff) return mi.writeIO(address, transform(data));
  if(address <= 0x044f'ffff) return vi.writeIO(address, transform(data));
  if(address <= 0x045f'ffff) return ai.writeIO(address, transform(data));
  if(address <= 0x046f'ffff) return pi.writeIO(address, transform(data));
  if(address <= 0x047f'ffff) return ri.writeIO(address, transform(data));
  if(address <= 0x048f'ffff) return si.writeIO(address, transform(data));
  if(address <= 0x0fff'ffff) return;
  if(address <= 0x1fbf'ffff) return;
  if(address <= 0x1fc0'07bf) return;
  if(address <= 0x1fc0'07ff) return pi.ram.writeHalf(address, data);
  return;
}

inline auto Bus::writeWord(u32 address, u32 data) -> void {
  address &= 0x1fff'fffc;
  if(address <= 0x007f'ffff) return rdram.ram.writeWord(address, data);
  if(address <= 0x03ef'ffff) return;
  if(address <= 0x03ff'ffff) return rdram.writeIO(address, data);
  if(address <= 0x0400'0fff) return rsp.dmem.writeWord(address, data);
  if(address <= 0x0400'1fff) return rsp.imem.writeWord(address, data);
  if(address <= 0x0403'ffff) return;
  if(address <= 0x0407'ffff) return rsp.writeSCC(address, data);
  if(address <= 0x040f'ffff) return rsp.writeIO(address, data);
  if(address <= 0x041f'ffff) return rdp.writeSCC(address, data);
  if(address <= 0x042f'ffff) return rdp.writeIO(address, data);
  if(address <= 0x043f'ffff) return mi.writeIO(address, data);
  if(address <= 0x044f'ffff) return vi.writeIO(address, data);
  if(address <= 0x045f'ffff) return ai.writeIO(address, data);
  if(address <= 0x046f'ffff) return pi.writeIO(address, data);
  if(address <= 0x047f'ffff) return ri.writeIO(address, data);
  if(address <= 0x048f'ffff) return si.writeIO(address, data);
  if(address <= 0x0fff'ffff) return;
  if(address <= 0x1fbf'ffff) return;
  if(address <= 0x1fc0'07bf) return;
  if(address <= 0x1fc0'07ff) return pi.ram.writeWord(address, data);
  return;
}

inline auto Bus::writeDouble(u32 address, u64 data) -> void {
  address &= 0x1fff'fff8;
  if(address <= 0x007f'ffff) return rdram.ram.writeDouble(address, data);
  if(address <= 0x03ef'ffff) return;
  if(address <= 0x03ff'ffff) { rdram.writeIO(address, data >> 32); return rdram.writeIO(address + 4, data); }
  if(address <= 0x0400'0fff) return rsp.dmem.writeDouble(address, data);
  if(address <= 0x0400'1fff) return rsp.imem.writeDouble(address, data);
  if(address <= 0x0403'ffff) return;
  if(address <= 0x0407'ffff) { rsp.writeSCC (address, data >> 32); return rsp.writeSCC (address + 4, data); }
  if(address <= 0x040f'ffff) { rsp.writeIO  (address, data >> 32); return rsp.writeIO  (address + 4, data); }
  if(address <= 0x041f'ffff) { rdp.writeSCC (address, data >> 32); return rdp.writeSCC (address + 4, data); }
  if(address <= 0x042f'ffff) { rdp.writeIO  (address, data >> 32); return rdp.writeIO  (address + 4, data); }
  if(address <= 0x043f'ffff) { mi.writeIO   (address, data >> 32); return mi.writeIO   (address + 4, data); }
  if(address <= 0x044f'ffff) { vi.writeIO   (address, data >> 32); return vi.writeIO   (address + 4, data); }
  if(address <= 0x045f'ffff) { ai.writeIO   (address, data >> 32); return ai.writeIO   (address + 4, data); }
  if(address <= 0x046f'ffff) { pi.writeIO   (address, data >> 32); return pi.writeIO   (address + 4, data); }
  if(address <= 0x047f'ffff) { ri.writeIO   (address, data >> 32); return ri.writeIO   (address + 4, data); }
  if(address <= 0x048f'ffff) { si.writeIO   (address, data >> 32); return si.writeIO   (address + 4, data); }
  if(address <= 0x0fff'ffff) return;
  if(address <= 0x1fbf'ffff) return;
  if(address <= 0x1fc0'07bf) return;
  if(address <= 0x1fc0'07ff) return pi.ram.writeDouble(address, data);
  return;
}
