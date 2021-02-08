auto SPC700::algorithmADC(n8 x, n8 y) -> n8 {
  s32 z = x + y + CF;
  CF = z > 0xff;
  ZF = (n8)z == 0;
  HF = (x ^ y ^ z) & 0x10;
  VF = ~(x ^ y) & (x ^ z) & 0x80;
  NF = z & 0x80;
  return z;
}

auto SPC700::algorithmAND(n8 x, n8 y) -> n8 {
  x &= y;
  ZF = x == 0;
  NF = x & 0x80;
  return x;
}

auto SPC700::algorithmASL(n8 x) -> n8 {
  CF = x & 0x80;
  x <<= 1;
  ZF = x == 0;
  NF = x & 0x80;
  return x;
}

auto SPC700::algorithmCMP(n8 x, n8 y) -> n8 {
  s32 z = x - y;
  CF = z >= 0;
  ZF = (n8)z == 0;
  NF = z & 0x80;
  return x;
}

auto SPC700::algorithmDEC(n8 x) -> n8 {
  x--;
  ZF = x == 0;
  NF = x & 0x80;
  return x;
}

auto SPC700::algorithmEOR(n8 x, n8 y) -> n8 {
  x ^= y;
  ZF = x == 0;
  NF = x & 0x80;
  return x;
}

auto SPC700::algorithmINC(n8 x) -> n8 {
  x++;
  ZF = x == 0;
  NF = x & 0x80;
  return x;
}

auto SPC700::algorithmLD(n8 x, n8 y) -> n8 {
  ZF = y == 0;
  NF = y & 0x80;
  return y;
}

auto SPC700::algorithmLSR(n8 x) -> n8 {
  CF = x & 0x01;
  x >>= 1;
  ZF = x == 0;
  NF = x & 0x80;
  return x;
}

auto SPC700::algorithmOR(n8 x, n8 y) -> n8 {
  x |= y;
  ZF = x == 0;
  NF = x & 0x80;
  return x;
}

auto SPC700::algorithmROL(n8 x) -> n8 {
  bool carry = CF;
  CF = x & 0x80;
  x = x << 1 | carry;
  ZF = x == 0;
  NF = x & 0x80;
  return x;
}

auto SPC700::algorithmROR(n8 x) -> n8 {
  bool carry = CF;
  CF = x & 0x01;
  x = carry << 7 | x >> 1;
  ZF = x == 0;
  NF = x & 0x80;
  return x;
}

auto SPC700::algorithmSBC(n8 x, n8 y) -> n8 {
  return algorithmADC(x, ~y);
}

//

auto SPC700::algorithmADW(n16 x, n16 y) -> n16 {
  n16 z;
  CF = 0;
  z  = algorithmADC(x, y);
  z |= algorithmADC(x >> 8, y >> 8) << 8;
  ZF = z == 0;
  return z;
}

auto SPC700::algorithmCPW(n16 x, n16 y) -> n16 {
  s32 z = x - y;
  CF = z >= 0;
  ZF = (n16)z == 0;
  NF = z & 0x8000;
  return x;
}

auto SPC700::algorithmLDW(n16 x, n16 y) -> n16 {
  ZF = y == 0;
  NF = y & 0x8000;
  return y;
}

auto SPC700::algorithmSBW(n16 x, n16 y) -> n16 {
  n16 z;
  CF = 1;
  z  = algorithmSBC(x, y);
  z |= algorithmSBC(x >> 8, y >> 8) << 8;
  ZF = z == 0;
  return z;
}
