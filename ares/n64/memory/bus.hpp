inline auto Bus::readByte(u32 address) -> u8 {
  address &= 0x1fff'ffff;
  if(address <= 0x007f'ffff) return rdram.ram.readByte(address);
  if(address <= 0x03ff'ffff) return 0;
  if(address <= 0x0400'0fff) return rsp.dmem.readByte(address);
  if(address <= 0x0400'1fff) return rsp.imem.readByte(address);
  if(address <= 0x046f'ffff) return 0;
  if(address <= 0x047f'ffff) return rdram.readIO(address & ~3);
  if(address <= 0x0fff'ffff) return 0;
  if(address <= 0x17ff'ffff) return cartridge.rom.readByte(address);
  return 0;
}

inline auto Bus::readHalf(u32 address) -> u16 {
  address &= 0x1fff'ffff;
  if(address <= 0x007f'ffff) return rdram.ram.readHalf(address);
  if(address <= 0x03ff'ffff) return 0;
  if(address <= 0x0400'0fff) return rsp.dmem.readHalf(address);
  if(address <= 0x0400'1fff) return rsp.imem.readHalf(address);
  if(address <= 0x046f'ffff) return 0;
  if(address <= 0x047f'ffff) return rdram.readIO(address & ~1);
  if(address <= 0x0fff'ffff) return 0;
  if(address <= 0x17ff'ffff) return cartridge.rom.readHalf(address);
  return 0;
}

inline auto Bus::readWord(u32 address) -> u32 {
  address &= 0x1fff'ffff;
  if(address <= 0x007f'ffff) return rdram.ram.readWord(address);
  if(address <= 0x03ff'ffff) return 0;
  if(address <= 0x0400'0fff) return rsp.dmem.readWord(address);
  if(address <= 0x0400'1fff) return rsp.imem.readWord(address);
  if(address <= 0x046f'ffff) return 0;
  if(address <= 0x047f'ffff) return rdram.readIO(address);
  if(address <= 0x0fff'ffff) return 0;
  if(address <= 0x17ff'ffff) return cartridge.rom.readWord(address);
  return 0;
}

inline auto Bus::readDouble(u32 address) -> u64 {
  address &= 0x1fff'ffff;
  if(address <= 0x07ff'ffff) return rdram.ram.readDouble(address);
  if(address <= 0x03ff'ffff) return 0;
  if(address <= 0x0400'0fff) return rsp.dmem.readDouble(address);
  if(address <= 0x0400'1fff) return rsp.imem.readDouble(address);
  if(address <= 0x046f'ffff) return 0;
  if(address <= 0x047f'ffff) { u64 data = rdram.readIO(address) << 32; return data | rdram.readIO(address + 4); }
  if(address <= 0x0fff'ffff) return 0;
  if(address <= 0x17ff'ffff) return cartridge.rom.readDouble(address);
  return 0;
}

inline auto Bus::writeByte(u32 address, u8 data) -> void {
  address &= 0x1fff'ffff;
  if(address <= 0x007f'ffff) return rdram.ram.writeByte(address, data);
  if(address <= 0x03ff'ffff) return;
  if(address <= 0x0400'0fff) return rsp.dmem.writeByte(address, data);
  if(address <= 0x0400'1fff) return rsp.imem.writeByte(address, data);
  if(address <= 0x046f'ffff) return;
  if(address <= 0x047f'ffff) return rdram.writeIO(address & ~3, data);
  if(address <= 0x0fff'ffff) return;
  if(address <= 0x17ff'ffff) return;
  return;
}

inline auto Bus::writeHalf(u32 address, u16 data) -> void {
  address &= 0x1fff'ffff;
  if(address <= 0x007f'ffff) return rdram.ram.writeHalf(address, data);
  if(address <= 0x03ff'ffff) return;
  if(address <= 0x0400'0fff) return rsp.dmem.writeHalf(address, data);
  if(address <= 0x0400'1fff) return rsp.imem.writeHalf(address, data);
  if(address <= 0x046f'ffff) return;
  if(address <= 0x047f'ffff) return rdram.writeIO(address & ~1, data);
  if(address <= 0x0fff'ffff) return;
  if(address <= 0x17ff'ffff) return;
  return;
}

inline auto Bus::writeWord(u32 address, u32 data) -> void {
  address &= 0x1fff'ffff;
  if(address <= 0x007f'ffff) return rdram.ram.writeWord(address, data);
  if(address <= 0x003f'ffff) return;
  if(address <= 0x0400'0fff) return rsp.dmem.writeWord(address, data);
  if(address <= 0x0400'1fff) return rsp.imem.writeWord(address, data);
  if(address <= 0x046f'ffff) return;
  if(address <= 0x047f'ffff) return rdram.writeIO(address, data);
  if(address <= 0x0fff'ffff) return;
  if(address <= 0x17ff'ffff) return;
  return;
}

inline auto Bus::writeDouble(u32 address, u64 data) -> void {
  address &= 0x1fff'ffff;
  if(address <= 0x007f'ffff) return rdram.ram.writeDouble(address, data);
  if(address <= 0x003f'ffff) return;
  if(address <= 0x0400'0fff) return rsp.dmem.writeDouble(address, data);
  if(address <= 0x0400'1fff) return rsp.imem.writeDouble(address, data);
  if(address <= 0x046f'ffff) return;
  if(address <= 0x047f'ffff) { rdram.writeIO(address, data >> 32); return rdram.writeIO(address + 4, data); }
  if(address <= 0x0fff'ffff) return;
  if(address <= 0x17ff'ffff) return;
  return;
}
