/* Vaguely based on Aes.c -- AES encryption / decryption
2017-01-24 : Igor Pavlov : Public domain */

#include <n64/n64.hpp>

namespace ares::Nintendo64 {

AES aes;

static const u8 InvS[256] = {
  0x52, 0x09, 0x6A, 0xD5, 0x30, 0x36, 0xA5, 0x38, 0xBF, 0x40, 0xA3, 0x9E, 0x81, 0xF3, 0xD7, 0xFB,
  0x7C, 0xE3, 0x39, 0x82, 0x9B, 0x2F, 0xFF, 0x87, 0x34, 0x8E, 0x43, 0x44, 0xC4, 0xDE, 0xE9, 0xCB,
  0x54, 0x7B, 0x94, 0x32, 0xA6, 0xC2, 0x23, 0x3D, 0xEE, 0x4C, 0x95, 0x0B, 0x42, 0xFA, 0xC3, 0x4E,
  0x08, 0x2E, 0xA1, 0x66, 0x28, 0xD9, 0x24, 0xB2, 0x76, 0x5B, 0xA2, 0x49, 0x6D, 0x8B, 0xD1, 0x25,
  0x72, 0xF8, 0xF6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xD4, 0xA4, 0x5C, 0xCC, 0x5D, 0x65, 0xB6, 0x92,
  0x6C, 0x70, 0x48, 0x50, 0xFD, 0xED, 0xB9, 0xDA, 0x5E, 0x15, 0x46, 0x57, 0xA7, 0x8D, 0x9D, 0x84,
  0x90, 0xD8, 0xAB, 0x00, 0x8C, 0xBC, 0xD3, 0x0A, 0xF7, 0xE4, 0x58, 0x05, 0xB8, 0xB3, 0x45, 0x06,
  0xD0, 0x2C, 0x1E, 0x8F, 0xCA, 0x3F, 0x0F, 0x02, 0xC1, 0xAF, 0xBD, 0x03, 0x01, 0x13, 0x8A, 0x6B,
  0x3A, 0x91, 0x11, 0x41, 0x4F, 0x67, 0xDC, 0xEA, 0x97, 0xF2, 0xCF, 0xCE, 0xF0, 0xB4, 0xE6, 0x73,
  0x96, 0xAC, 0x74, 0x22, 0xE7, 0xAD, 0x35, 0x85, 0xE2, 0xF9, 0x37, 0xE8, 0x1C, 0x75, 0xDF, 0x6E,
  0x47, 0xF1, 0x1A, 0x71, 0x1D, 0x29, 0xC5, 0x89, 0x6F, 0xB7, 0x62, 0x0E, 0xAA, 0x18, 0xBE, 0x1B,
  0xFC, 0x56, 0x3E, 0x4B, 0xC6, 0xD2, 0x79, 0x20, 0x9A, 0xDB, 0xC0, 0xFE, 0x78, 0xCD, 0x5A, 0xF4,
  0x1F, 0xDD, 0xA8, 0x33, 0x88, 0x07, 0xC7, 0x31, 0xB1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xEC, 0x5F,
  0x60, 0x51, 0x7F, 0xA9, 0x19, 0xB5, 0x4A, 0x0D, 0x2D, 0xE5, 0x7A, 0x9F, 0x93, 0xC9, 0x9C, 0xEF,
  0xA0, 0xE0, 0x3B, 0x4D, 0xAE, 0x2A, 0xF5, 0xB0, 0xC8, 0xEB, 0xBB, 0x3C, 0x83, 0x53, 0x99, 0x61,
  0x17, 0x2B, 0x04, 0x7E, 0xBA, 0x77, 0xD6, 0x26, 0xE1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0C, 0x7D,
};

static inline u32 U32(u8 a0, u8 a1, u8 a2, u8 a3) {
  return (a0 << 24) | (a1 << 16) | (a2 << 8) | (a3 << 0);
}

template<u32 n>
static inline u8 GB(u32 x) {
  return (x >> (24 - 8 * n)) & 0xFF;
}

AES::AES() {
#define xtime(x) ((((x) << 1) ^ (((x) & 0x80) ? 0x1B : 0)) & 0xFF)
  for (auto i : range(256)) {
    u32 a1 = InvS[i];
    u32 a2 = xtime(a1);
    u32 a4 = xtime(a2);
    u32 a8 = xtime(a4);
    u32 a9 = a8 ^ a1;
    u32 aB = a8 ^ a2 ^ a1;
    u32 aD = a8 ^ a4 ^ a1;
    u32 aE = a8 ^ a4 ^ a2;
    D[0][i] = U32(aE, a9, aD, aB);
    D[1][i] = U32(aB, aE, a9, aD);
    D[2][i] = U32(aD, aB, aE, a9);
    D[3][i] = U32(a9, aD, aB, aE);
  }
#undef xtime
}

auto AES::decodeCBC(Memory::Writable &mem, u32 pos, u32 numBlocks) -> void {
  if constexpr(Accuracy::AES::SIMD) {
    __m128i iv = mvIV;

    for (; numBlocks >= 4; numBlocks -= 4, pos += 4 * AES_BLOCK_SIZE) {
      __m128i *key = mvKey;
      __m128i i0, i1, i2, i3;
      __m128i m0, m1, m2, m3;
      __m128i t;

      i0 = read128(mem, pos + 0 * AES_BLOCK_SIZE);
      i1 = read128(mem, pos + 1 * AES_BLOCK_SIZE);
      i2 = read128(mem, pos + 2 * AES_BLOCK_SIZE);
      i3 = read128(mem, pos + 3 * AES_BLOCK_SIZE);

      t = *key++;
      m0 = _mm_xor_si128(i0, t);
      m1 = _mm_xor_si128(i1, t);
      m2 = _mm_xor_si128(i2, t);
      m3 = _mm_xor_si128(i3, t);

      for (auto r : range(9)) {
        t = *key++;
        m0 = _mm_aesdec_si128(m0, t);
        m1 = _mm_aesdec_si128(m1, t);
        m2 = _mm_aesdec_si128(m2, t);
        m3 = _mm_aesdec_si128(m3, t);
      }
      t = *key++;
      m0 = _mm_aesdeclast_si128(m0, t);
      m1 = _mm_aesdeclast_si128(m1, t);
      m2 = _mm_aesdeclast_si128(m2, t);
      m3 = _mm_aesdeclast_si128(m3, t);

      t = _mm_xor_si128(m0, iv); iv = i0; write128(mem, pos + 0 * AES_BLOCK_SIZE, t);
      t = _mm_xor_si128(m1, iv); iv = i1; write128(mem, pos + 1 * AES_BLOCK_SIZE, t);
      t = _mm_xor_si128(m2, iv); iv = i2; write128(mem, pos + 2 * AES_BLOCK_SIZE, t);
      t = _mm_xor_si128(m3, iv); iv = i3; write128(mem, pos + 3 * AES_BLOCK_SIZE, t);
    }

    while (numBlocks--) {
      __m128i *key = mvKey;
      __m128i i = read128(mem, pos);
      __m128i m = _mm_xor_si128(*key++, i);

      for (auto r : range(9))
        m = _mm_aesdec_si128(m, *key++);
      m = _mm_aesdeclast_si128(m, *key++);
  
      m = _mm_xor_si128(m, iv); iv = i; write128(mem, pos, m);
      pos += AES_BLOCK_SIZE;
    }

    mvIV = iv;
  }

  if constexpr(Accuracy::AES::SISD) {
    for (; numBlocks != 0; numBlocks--, pos += AES_BLOCK_SIZE) {
      const u32 *key = mKey;
      u32 out[4];
      u32 in[4];
      u32 s[4];
      u32 t[4];

      in[0] = mem.read<Word>(pos + 0 * 4);
      in[1] = mem.read<Word>(pos + 1 * 4);
      in[2] = mem.read<Word>(pos + 2 * 4);
      in[3] = mem.read<Word>(pos + 3 * 4);

      s[0] = in[0] ^ key[0];
      s[1] = in[1] ^ key[1];
      s[2] = in[2] ^ key[2];
      s[3] = in[3] ^ key[3];
      key += 4;

#define HD(s, i, x) D[x][GB<x>(s[(i - x) & 3])]
#define HD4(s, i) (HD(s, i, 0) ^ HD(s, i, 1) ^ HD(s, i, 2) ^ HD(s, i, 3))
      for (auto r : range(4)) {
        t[0] = HD4(s, 0) ^ key[0];
        t[1] = HD4(s, 1) ^ key[1];
        t[2] = HD4(s, 2) ^ key[2];
        t[3] = HD4(s, 3) ^ key[3];
        key += 4;
        s[0] = HD4(t, 0) ^ key[0];
        s[1] = HD4(t, 1) ^ key[1];
        s[2] = HD4(t, 2) ^ key[2];
        s[3] = HD4(t, 3) ^ key[3];
        key += 4;
      }
      t[0] = HD4(s, 0) ^ key[0];
      t[1] = HD4(s, 1) ^ key[1];
      t[2] = HD4(s, 2) ^ key[2];
      t[3] = HD4(s, 3) ^ key[3];
      key += 4;
#undef HD4
#undef HD

#define FD(s, i, x) InvS[GB<x>(s[(i - x) & 3])]
#define FD4(s, i) U32(FD(s, i, 0), FD(s, i, 1), FD(s, i, 2), FD(s, i, 3))
      out[0] = mIV[0] ^ FD4(t, 0) ^ key[0];
      out[1] = mIV[1] ^ FD4(t, 1) ^ key[1];
      out[2] = mIV[2] ^ FD4(t, 2) ^ key[2];
      out[3] = mIV[3] ^ FD4(t, 3) ^ key[3];
#undef FD4
#undef FD

      mIV[0] = in[0];
      mIV[1] = in[1];
      mIV[2] = in[2];
      mIV[3] = in[3];

      mem.write<Word>(pos + 0 * 4, out[0]);
      mem.write<Word>(pos + 1 * 4, out[1]);
      mem.write<Word>(pos + 2 * 4, out[2]);
      mem.write<Word>(pos + 3 * 4, out[3]);
    }
  }
}

}
