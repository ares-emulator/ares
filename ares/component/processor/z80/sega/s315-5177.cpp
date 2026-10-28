// Sega 315-5177 decryption algorithm derived from MAME (BSD-3-Clause).
// See the MAME section in LICENSE.
auto S315_5177::decrypt(n16 address, n8 source, bool opcode) const -> n8 {
  if(address >= 0x8000) return source;

  static constexpr u8 permutations[24][4] = {
    {6,4,2,0}, {4,6,2,0}, {2,4,6,0}, {0,4,2,6},
    {6,2,4,0}, {6,0,2,4}, {6,4,0,2}, {2,6,4,0},
    {4,2,6,0}, {4,6,0,2}, {6,0,4,2}, {0,6,4,2},
    {4,0,6,2}, {0,4,6,2}, {6,2,0,4}, {2,6,0,4},
    {0,6,2,4}, {2,0,6,4}, {0,2,6,4}, {4,2,0,6},
    {2,4,0,6}, {4,0,2,6}, {2,0,4,6}, {0,2,4,6},
  };

  static constexpr u8 pattern[24] = {
    0x04,0x54,0x51,0x15,0x40,0x44,0x01,0x51,
    0x55,0x10,0x44,0x41,0x05,0x55,0x50,0x14,
    0x41,0x45,0x00,0x50,0x54,0x11,0x45,0x40,
  };

  static constexpr u8 permutationPattern[64] = {
     0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3,
     3, 3, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 6, 6, 6, 6,
     6, 7, 7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9, 9,10,
    10,10,10,10,11,11,11,11,11,12,12,12,12,12,13,13,
  };

  n6 row = 0;
  row.bit(0) = address.bit(0);
  row.bit(1) = address.bit(3);
  row.bit(2) = address.bit(6);
  row.bit(3) = address.bit(9);
  row.bit(4) = address.bit(12);
  row.bit(5) = address.bit(14);

  n7 index = 0;
  index.bit(0) = !opcode;
  index.bit(1,6) = row;
  n6 patternIndex = index.bit(0,5);
  unsigned permutationIndex = permutationPattern[patternIndex] + (index.bit(6) ? 8 : 0);
  n8 mask = pattern[patternIndex % 24];
  if(permutationIndex == 0) return source ^ mask;
  const u8* positions = permutations[permutationIndex];

  n8 result = 0;
  result.bit(7) = source.bit(7);
  result.bit(6) = source.bit(positions[0]);
  result.bit(5) = source.bit(5);
  result.bit(4) = source.bit(positions[1]);
  result.bit(3) = source.bit(3);
  result.bit(2) = source.bit(positions[2]);
  result.bit(1) = source.bit(1);
  result.bit(0) = source.bit(positions[3]);
  return result ^ mask;
}
