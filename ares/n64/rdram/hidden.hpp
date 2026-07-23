struct HiddenRAM {
  u8* data = nullptr;

  auto nibble(u32 address) -> u32 {
    u8* h = &data[address >> 1];
    return (h[0] & 3) << 2 | (h[1] & 3);
  }

  auto writeBit(u32 address, n1 value) -> void {
    u32 shift = 1 - (address & 1);
    u8& h = data[address >> 1];
    h = (h & ~(1 << shift)) | (value << shift);
  }

  template<u32 Size>
  auto update(u32 address, u64 value) -> void {
    u8* h = &data[address >> 1];
    if constexpr(Size == Byte) {
      if(address & 1) writeBit(address, value & 1);
      else            writeBit(address, 0);
    }
    if constexpr(Size == Half) h[0] = (value & 1) * 3;
    if constexpr(Size == Word) {
      u16 packed = (value >> 16 & 1) * 3 | ((value & 1) * 3) << 8;
      memory::writel<2>(h, packed);
    }
    if constexpr(Size == Dual) {
      u32 packed = (u32)((value >> 48 & 1) * 3) <<  0
                 | (u32)((value >> 32 & 1) * 3) <<  8
                 | (u32)((value >> 16 & 1) * 3) << 16
                 | (u32)((value >>  0 & 1) * 3) << 24;
      memory::writel<4>(h, packed);
    }
  }

  template<u32 Size>
  auto ebusScatter(u32 address, u64 value) -> void {
    if constexpr(Size == Byte) {
      writeBit(address, (address & 3) == 3 ? value & 1 : 0);
    }
    if constexpr(Size == Half) {
      if(address & 2) {
        writeBit(address + 0, value >> 1 & 1);
        writeBit(address + 1, value >> 0 & 1);
      } else {
        writeBit(address + 0, 0);
        writeBit(address + 1, 0);
      }
    }
    if constexpr(Size == Word) {
      writeBit(address + 0, value >> 3 & 1);
      writeBit(address + 1, value >> 2 & 1);
      writeBit(address + 2, value >> 1 & 1);
      writeBit(address + 3, value >> 0 & 1);
    }
    if constexpr(Size == Dual) {
      ebusScatter<Word>(address + 0, value >> 32);
      ebusScatter<Word>(address + 4, value >>  0);
    }
  }

  auto updateWords4(u32 address, const u32* value) -> void {
    #if ARCHITECTURE_SUPPORTS_SSE4_1
    __m128i v = _mm_loadu_si128((const __m128i*)value);
    __m128i m = _mm_and_si128(v, _mm_set1_epi16(1));
    m = _mm_or_si128(m, _mm_slli_epi16(m, 1));
    static const __m128i sel = _mm_setr_epi8(2, 0, 6, 4, 10, 8, 14, 12, -1, -1, -1, -1, -1, -1, -1, -1);
    _mm_storel_epi64((__m128i*)&data[address >> 1], _mm_shuffle_epi8(m, sel));
    #else
    u8* h = &data[address >> 1];
    for(u32 n : range(4)) {
      h[n * 2 + 0] = (value[n] >> 16 & 1) * 3;
      h[n * 2 + 1] = (value[n] >>  0 & 1) * 3;
    }
    #endif
  }

  template<u32 Size>
  auto updateBurst(u32 address, const u32* value) -> void {
    updateWords4(address, value);
    if constexpr(Size == ICache) updateWords4(address + 16, value + 4);
  }
};
