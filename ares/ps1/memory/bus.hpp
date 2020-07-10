#define decode(write, access, ...) \
  if(address >= 0xfffe'0000) return unmapped; \
  address &= 0x1fff'ffff; \
  if(address <= 0x001f'ffff) return cpu.ram.access(__VA_ARGS__); \
  if(address <= 0x1eff'ffff) return unmapped; \
  if(address <= 0x1f7f'ffff) return unmapped; \
  if(address <= 0x1f80'03ff) return cpu.cache.access(__VA_ARGS__); \
  if(address <= 0x1f80'103f) return unmapped; \
  if(address <= 0x1f80'104f) return peripheral.access(__VA_ARGS__); \
  if(address <= 0x1f80'106f) return unmapped; \
  if(address <= 0x1f80'107f) return interrupt.access(__VA_ARGS__); \
  if(address <= 0x1f80'10ff) return dma.access(__VA_ARGS__); \
  if(address <= 0x1f80'112f) return timer.access(__VA_ARGS__); \
  if(address <= 0x1f80'17ff) return unmapped; \
  if(address <= 0x1f80'180f) return disc.access(__VA_ARGS__); \
  if(address <= 0x1f80'181f) return gpu.access(__VA_ARGS__); \
  if(address <= 0x1f80'1bff) return unmapped; \
  if(address <= 0x1f80'1fff) return spu.access(__VA_ARGS__); \
  if(address <= 0x1fbf'ffff) return unmapped; \
  if(address <= 0x1fff'ffff) return bios.access(__VA_ARGS__); \
  return unmapped; \

#define unmapped 0

inline auto Bus::readByte(u32 address) -> u8 {
  decode(0, readByte, address);
}

inline auto Bus::readHalf(u32 address) -> u16 {
  decode(0, readHalf, address);
}

inline auto Bus::readWord(u32 address) -> u32 {
  decode(0, readWord, address);
}

#undef unmapped
#define unmapped

inline auto Bus::writeByte(u32 address, u8 data) -> void {
  decode(1, writeByte, address, data);
}

inline auto Bus::writeHalf(u32 address, u16 data) -> void {
  decode(1, writeHalf, address, data);
}

inline auto Bus::writeWord(u32 address, u32 data) -> void {
  decode(1, writeWord, address, data);
}

#undef unmapped
#undef decode
