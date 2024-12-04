//iQue Player AES-128-CBC Decryption

class AES {
public:
  AES();

  static constexpr u32 AES_BLOCK_SIZE = 0x10;

  static inline auto swap128(__m128i &v) -> __m128i {
    return _mm_shuffle_epi8(v, _mm_set_epi8(12,13,14,15, 8,9,10,11, 4,5,6,7, 0,1,2,3));
  }

  static inline auto read128(Memory::Writable &mem, u32 p) -> __m128i {
    return swap128(*(__m128i *)&mem.data[p & mem.maskWord]);
  }

  static inline auto write128(Memory::Writable &mem, u32 p, __m128i &v) -> void {
    *(__m128i *)&mem.data[p & mem.maskWord] = swap128(v);
  }

  inline auto setKey(Memory::Writable& key) -> void {
    if constexpr(Accuracy::AES::SIMD) {
      for (auto i : range(0xB0 / 0x10))
        mvKey[i] = read128(key, 0x420 + i * 0x10);
    }

    if constexpr(Accuracy::AES::SISD) {
      for (auto i : range(0xB0 / 4))
        mKey[i] = key.read<Word>(0x420 + i * 4);
    }
  }

  inline auto setIV(Memory::Writable& iv, u32 offset) -> void {
    if constexpr(Accuracy::AES::SIMD) {
      mvIV = read128(iv, offset);
    }

    if constexpr(Accuracy::AES::SISD) {
      for (auto i : range(0x10 / 4))
        mIV[i] = iv.read<Word>(offset + i * 4);
    }
  }

  auto decodeCBC(Memory::Writable& mem, u32 pos, u32 numBlocks) -> void;

private:
  u32 D[4][256];
  union {
    u32 mIV[0x10 / 4];
    __m128i mvIV;
  };
  union {
    u32 mKey[0xB0 / 4];
    __m128i mvKey[0xB0 / 0x10];
  };
};

extern AES aes;
