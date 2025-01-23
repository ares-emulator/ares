
namespace ECC {

static constexpr u8 ECCTable[256] = {
  0x00, 0x55, 0x59, 0x0C, 0x65, 0x30, 0x3C, 0x69, 0x69, 0x3C, 0x30, 0x65, 0x0C, 0x59, 0x55, 0x00,
  0x95, 0xC0, 0xCC, 0x99, 0xF0, 0xA5, 0xA9, 0xFC, 0xFC, 0xA9, 0xA5, 0xF0, 0x99, 0xCC, 0xC0, 0x95,
  0x99, 0xCC, 0xC0, 0x95, 0xFC, 0xA9, 0xA5, 0xF0, 0xF0, 0xA5, 0xA9, 0xFC, 0x95, 0xC0, 0xCC, 0x99,
  0x0C, 0x59, 0x55, 0x00, 0x69, 0x3C, 0x30, 0x65, 0x65, 0x30, 0x3C, 0x69, 0x00, 0x55, 0x59, 0x0C,
  0xA5, 0xF0, 0xFC, 0xA9, 0xC0, 0x95, 0x99, 0xCC, 0xCC, 0x99, 0x95, 0xC0, 0xA9, 0xFC, 0xF0, 0xA5,
  0x30, 0x65, 0x69, 0x3C, 0x55, 0x00, 0x0C, 0x59, 0x59, 0x0C, 0x00, 0x55, 0x3C, 0x69, 0x65, 0x30,
  0x3C, 0x69, 0x65, 0x30, 0x59, 0x0C, 0x00, 0x55, 0x55, 0x00, 0x0C, 0x59, 0x30, 0x65, 0x69, 0x3C,
  0xA9, 0xFC, 0xF0, 0xA5, 0xCC, 0x99, 0x95, 0xC0, 0xC0, 0x95, 0x99, 0xCC, 0xA5, 0xF0, 0xFC, 0xA9,
  0xA9, 0xFC, 0xF0, 0xA5, 0xCC, 0x99, 0x95, 0xC0, 0xC0, 0x95, 0x99, 0xCC, 0xA5, 0xF0, 0xFC, 0xA9,
  0x3C, 0x69, 0x65, 0x30, 0x59, 0x0C, 0x00, 0x55, 0x55, 0x00, 0x0C, 0x59, 0x30, 0x65, 0x69, 0x3C,
  0x30, 0x65, 0x69, 0x3C, 0x55, 0x00, 0x0C, 0x59, 0x59, 0x0C, 0x00, 0x55, 0x3C, 0x69, 0x65, 0x30,
  0xA5, 0xF0, 0xFC, 0xA9, 0xC0, 0x95, 0x99, 0xCC, 0xCC, 0x99, 0x95, 0xC0, 0xA9, 0xFC, 0xF0, 0xA5,
  0x0C, 0x59, 0x55, 0x00, 0x69, 0x3C, 0x30, 0x65, 0x65, 0x30, 0x3C, 0x69, 0x00, 0x55, 0x59, 0x0C,
  0x99, 0xCC, 0xC0, 0x95, 0xFC, 0xA9, 0xA5, 0xF0, 0xF0, 0xA5, 0xA9, 0xFC, 0x95, 0xC0, 0xCC, 0x99,
  0x95, 0xC0, 0xCC, 0x99, 0xF0, 0xA5, 0xA9, 0xFC, 0xFC, 0xA9, 0xA5, 0xF0, 0x99, 0xCC, 0xC0, 0x95,
  0x00, 0x55, 0x59, 0x0C, 0x65, 0x30, 0x3C, 0x69, 0x69, 0x3C, 0x30, 0x65, 0x0C, 0x59, 0x55, 0x00,
};

inline auto computePageECC(const u8 data[512], u8 ecc[8]) -> void {
  ecc[3] = ecc[4] = 0xFF;
  for (auto j : range(2)) {
    u32 ecc2 = 0;
    u32 l0 = 0, l1 = 0;
    for (auto i : range(256)) {
      // Lookup in the parity table
      u8 val = ECCTable[*data++];
      // If byte has odd parity, update the line parity 
      if (val & 1) {
        l0 ^= i;
        l1 ^= ~i;
        val ^= 1;
      }
      // Update column parity
      ecc2 ^= val;
    }
    // Interleave the bits of the two line parities (l0 and l1)
    l0 = (l0 | (l0 << 4)) & 0x0F0F;
    l0 = (l0 | (l0 << 2)) & 0x3333;
    l0 = (l0 | (l0 << 1)) & 0x5555;
    l1 &= 0xFF;
    l1 = (l1 | (l1 << 4)) & 0x0F0F;
    l1 = (l1 | (l1 << 2)) & 0x3333;
    l1 = (l1 | (l1 << 1)) & 0x5555;
    l0 = (l1 << 0) | (l0 << 1);
    // Store the inverted line parities and column parity
    auto k = j ^ 1;
    ecc[k * 4 + 1 - j] = ~l0 >> 0;
    ecc[k * 4 + 2 - j] = ~l0 >> 8;
    ecc[k * 4 + 3 - j] = ~ecc2;
  }
}

#if MEM_ECC
inline auto computePageECC(Memory::Writable& buffer, Memory::Writable& spare) -> void {
  spare.write<Byte>(8 + 3, 0xFF);
  spare.write<Byte>(8 + 4, 0xFF);
  for (auto j : range(2)) {
    u32 ecc2 = 0;
    u32 l0 = 0, l1 = 0;
    for (auto i : range(256)) {
      // Lookup the parity table (with vertical symmetry for the second half)
      u8 val = ECCTable[buffer.read<Byte>(0x100 * j + i)];
      // If byte has odd parity, update the line parity 
      if (val & 1) {
        l0 ^= i;
        l1 ^= ~i;
        val ^= 1;
      }
      // Update column parity
      ecc2 ^= val;
    }
    // Interleave the bits of the two line parities (l0 and l1)
    l0 = (l0 | (l0 << 4)) & 0x0F0F;
    l0 = (l0 | (l0 << 2)) & 0x3333;
    l0 = (l0 | (l0 << 1)) & 0x5555;
    l1 &= 0xFF;
    l1 = (l1 | (l1 << 4)) & 0x0F0F;
    l1 = (l1 | (l1 << 2)) & 0x3333;
    l1 = (l1 | (l1 << 1)) & 0x5555;
    l0 = (l1 << 0) | (l0 << 1);
    // Store the inverted line parities and column parity
    auto k = j ^ 1;
    spare.write<Byte>(8 + k * 4 + 1 - j, ~l0 >> 0);
    spare.write<Byte>(8 + k * 4 + 2 - j, ~l0 >> 8);
    spare.write<Byte>(8 + k * 4 + 3 - j, ~ecc2);
  }
}

inline auto computePageECC_2(Memory::Writable& buffer, b1 which, u8 ecc[8], u32 n) -> void {
  for (auto j = n; j < 2; j++) { //When n = 0, computes 5,6,7,0,1,2; when n = 1, computes 0,1,2
    u32 ecc2 = 0;
    u32 l0 = 0, l1 = 0;
    for (auto i : range(256)) {
      // Lookup the parity table (with vertical symmetry for the second half)
      u8 val = ECCTable[buffer.read<Byte>(0x200 * which + 0x100 * j + i)];
      // If byte has odd parity, update the line parity 
      if (val & 1) {
        l0 ^= i;
        l1 ^= ~i;
        val ^= 1;
      }
      // Update column parity
      ecc2 ^= val;
    }
    // Interleave the bits of the two line parities (l0 and l1)
    l0 = (l0 | (l0 << 4)) & 0x0F0F;
    l0 = (l0 | (l0 << 2)) & 0x3333;
    l0 = (l0 | (l0 << 1)) & 0x5555;
    l1 &= 0xFF;
    l1 = (l1 | (l1 << 4)) & 0x0F0F;
    l1 = (l1 | (l1 << 2)) & 0x3333;
    l1 = (l1 | (l1 << 1)) & 0x5555;
    l0 = (l1 << 0) | (l0 << 1);
    // Store the inverted line parities and column parity
    auto k = j ^ 1;
    ecc[k * 4 + 1 - j] = ~l0 >> 0;
    ecc[k * 4 + 2 - j] = ~l0 >> 8;
    ecc[k * 4 + 3 - j] = ~ecc2;
  }
}

static inline auto mortonDecode(u16 x) -> u8 {
    x = (x >> 1) & 0x5555;       // 0101010101010101
    x = (x | (x >> 1)) & 0x3333; // 0011001100110011
    x = (x | (x >> 2)) & 0x0F0F; // 0000111100001111
    x = (x | (x >> 4)) & 0x00FF; // 0000000011111111
    return x;
}

inline auto checkPageECC(Memory::Writable& buffer, n27 addr, b1 which, u32 pageOffset) -> n2 {
  u32 n = (pageOffset == 0x100);
  u8 storedECC[8];
  u8 computedECC[8];
  computePageECC_2(buffer, which, computedECC, n);
  for(auto i : range(8))
    storedECC[i] = buffer.read<Byte>(0x400 + which * 0x10 + 8 + i);

  u8 d[2][3];

  //5,6,7 are for the first 0x100 bytes
  d[0][0] = storedECC[5] ^ computedECC[5];
  d[0][1] = storedECC[6] ^ computedECC[6];
  d[0][2] = storedECC[7] ^ computedECC[7];
  //0,1,2 are for the second 0x100 bytes
  d[1][0] = storedECC[0] ^ computedECC[0];
  d[1][1] = storedECC[1] ^ computedECC[1];
  d[1][2] = storedECC[2] ^ computedECC[2];

  //TOVERIFY: If one half of the page has a single-bit error and the other
  //half has a double-bit error, are both bits flagged?
  n2 res = 0;
  for (auto i = n; i < 2; i++) { // If n = 1, only do the second half
    if ((d[i][0] | d[i][1] | d[i][2]) == 0)
      continue; // Data is good

    printf("Non-matching ecc for NAND addr %08X\n", u32(addr));
    printf("%02X %02X %02X | %02X %02X %02X\n", storedECC[0], storedECC[1], storedECC[2], storedECC[5], storedECC[6], storedECC[7]);
    printf("%02X %02X %02X | %02X %02X %02X\n", computedECC[0], computedECC[1], computedECC[2], computedECC[5], computedECC[6], computedECC[7]);

    if (((d[i][0] ^ (d[i][0] >> 1)) & 0x55) == 0x55 &&
        ((d[i][1] ^ (d[i][1] >> 1)) & 0x55) == 0x55 &&
        ((d[i][2] ^ (d[i][2] >> 1)) & 0x54) == 0x54) {
      //Single-bit error in at least one page
      res.bit(1) = 1;

      printf("Single-bit error\n");

      //Correct it
      u8 byteLoc = mortonDecode((d[i][1] << 8) | d[i][0]);
      u8 bitLoc = mortonDecode(d[i][2]) >> 1;
      auto addr = 0x200 * which + 0x100 * i + byteLoc;
      buffer.write<Byte>(addr, buffer.read<Byte>(addr) ^ (1 << bitLoc));
    } else {
      //Double-bit error in at least one page
      res.bit(0) = 1;
      //Uncorrectable
      
      printf("Double-bit error\n");
    }
  }
  return res;
}
#endif

}
