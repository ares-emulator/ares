#define decode(write, access, ...) \
  if(address <= 0x007f'ffff) return rdram.ram.access(__VA_ARGS__); \
  if(address <= 0x03ef'ffff) return unmapped; \
  if(address <= 0x03ff'ffff) return rdram.access(__VA_ARGS__); \
  if(address <= 0x0400'0fff) return rsp.dmem.access(__VA_ARGS__); \
  if(address <= 0x0400'1fff) { \
    if constexpr(write) { rsp.recompiler.invalidate(); } \
    return rsp.imem.access(__VA_ARGS__); \
  } \
  if(address <= 0x0403'ffff) return unmapped; \
  if(address <= 0x0407'ffff) return rsp.access(__VA_ARGS__); \
  if(address <= 0x040f'ffff) return rsp.status.access(__VA_ARGS__); \
  if(address <= 0x041f'ffff) return rdp.access(__VA_ARGS__); \
  if(address <= 0x042f'ffff) return rdp.io.access(__VA_ARGS__); \
  if(address <= 0x043f'ffff) return mi.access(__VA_ARGS__); \
  if(address <= 0x044f'ffff) return vi.access(__VA_ARGS__); \
  if(address <= 0x045f'ffff) return ai.access(__VA_ARGS__); \
  if(address <= 0x046f'ffff) return pi.access(__VA_ARGS__); \
  if(address <= 0x047f'ffff) return ri.access(__VA_ARGS__); \
  if(address <= 0x048f'ffff) return si.access(__VA_ARGS__); \
  if(address <= 0x04ff'ffff) return unmapped; \
  if(address <= 0x0500'03ff) return dd.c2s.access(__VA_ARGS__); \
  if(address <= 0x0500'04ff) return dd.ds.access(__VA_ARGS__); \
  if(address <= 0x0500'057f) return dd.access(__VA_ARGS__); \
  if(address <= 0x0500'05bf) return dd.ms.access(__VA_ARGS__); \
  if(address <= 0x05ff'ffff) return unmapped; \
  if(address <= 0x063f'ffff) return dd.iplrom.access(__VA_ARGS__); \
  if(address <= 0x0fff'ffff) return unmapped; \
  if(address <= 0x1fbf'ffff) return cartridge.rom.access(__VA_ARGS__); \
  if(address <= 0x1fc0'07bf) return pi.rom.access(__VA_ARGS__); \
  if(address <= 0x1fc0'07ff) return pi.ram.access(__VA_ARGS__); \
  return unmapped; \

#define unmapped 0

inline auto Bus::readByte(u32 address) -> u8 {
  address &= 0x1fff'ffff;
  decode(0, readByte, address);
}

inline auto Bus::readHalf(u32 address) -> u16 {
  address &= 0x1fff'fffe;
  decode(0, readHalf, address);
}

inline auto Bus::readWord(u32 address) -> u32 {
  address &= 0x1fff'fffc;
  decode(0, readWord, address);
}

inline auto Bus::readDouble(u32 address) -> u64 {
  address &= 0x1fff'fff8;
  decode(0, readDouble, address);
}

#undef unmapped
#define unmapped

inline auto Bus::writeByte(u32 address, u8 data) -> void {
  address &= 0x1fff'ffff;
  cpu.recompiler.invalidate(address);
  decode(1, writeByte, address, data);
}

inline auto Bus::writeHalf(u32 address, u16 data) -> void {
  address &= 0x1fff'fffe;
  cpu.recompiler.invalidate(address);
  decode(1, writeHalf, address, data);
}

inline auto Bus::writeWord(u32 address, u32 data) -> void {
  address &= 0x1fff'fffc;
  cpu.recompiler.invalidate(address);
  decode(1, writeWord, address, data);
}

inline auto Bus::writeDouble(u32 address, u64 data) -> void {
  address &= 0x1fff'fff8;
  cpu.recompiler.invalidate(address + 0);
  cpu.recompiler.invalidate(address + 4);
  decode(2, writeDouble, address, data);
}

#undef unmapped
#undef decode
