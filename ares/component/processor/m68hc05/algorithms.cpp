auto M68HC05::algorithmADC(n8 x, n8 y) -> n8 {
  n8 z = x + y + CCR.C;
  n8 c = x ^ y ^ z;
  n8 o = (x ^ z) & (y ^ z);
  CCR.C = (c ^ o) & 0x80;
  CCR.Z = z == 0;
  CCR.N = z & 0x80;
  CCR.H = c & 0x10;
  return z;
}

auto M68HC05::algorithmADD(n8 x, n8 y) -> n8 {
  n8 z = x + y;
  n8 c = x ^ y ^ z;
  n8 o = (x ^ z) & (y ^ z);
  CCR.C = (c ^ o) & 0x80;
  CCR.Z = z == 0;
  CCR.N = z & 0x80;
  CCR.H = c & 0x10;
  return z;
}

auto M68HC05::algorithmAND(n8 x, n8 y) -> n8 {
  x = x & y;
  CCR.Z = x == 0;
  CCR.N = x & 0x80;
  return x;
}

auto M68HC05::algorithmASR(n8 x) -> n8 {
  CCR.C = x & 1;
  x = (i8)x >> 1;
  CCR.Z = x == 0;
  CCR.N = x & 0x80;
  return x;
}

auto M68HC05::algorithmBIT(n8 x, n8 y) -> n8 {
  n8 z = x & y;
  CCR.Z = z == 0;
  CCR.N = z & 0x80;
  return x;
}

auto M68HC05::algorithmCLR(n8 x) -> n8 {
  x = 0;
  CCR.Z = 1;
  CCR.N = 0;
  return x;
}

auto M68HC05::algorithmCMP(n8 x, n8 y) -> n8 {
  n8 z = x - y;
  n8 c = x ^ ~y ^ z;
  n8 o = (x ^ z) & (y ^ x);
  CCR.C = (c ^ o) & 0x80;
  CCR.Z = z == 0;
  CCR.N = z & 0x80;
  return x;
}

auto M68HC05::algorithmCOM(n8 x) -> n8 {
  x = ~x;
  CCR.C = 1;
  CCR.Z = x == 0;
  CCR.N = x & 0x80;
  return x;
}

auto M68HC05::algorithmDEC(n8 x) -> n8 {
  x--;
  CCR.Z = x == 0;
  CCR.N = x & 0x80;
  return x;
}

auto M68HC05::algorithmEOR(n8 x, n8 y) -> n8 {
  x = x ^ y;
  CCR.Z = x == 0;
  CCR.N = x & 0x80;
  return x;
}

auto M68HC05::algorithmINC(n8 x) -> n8 {
  x++;
  CCR.Z = x == 0;
  CCR.N = x & 0x80;
  return x;
}

auto M68HC05::algorithmLD(n8 x, n8 y) -> n8 {
  x = y;
  CCR.Z = x == 0;
  CCR.N = x & 0x80;
  return x;
}

auto M68HC05::algorithmLSL(n8 x) -> n8 {
  CCR.C = x >> 7;
  x = x << 7;
  CCR.Z = x == 0;
  CCR.N = x & 0x80;
  return x;
}

auto M68HC05::algorithmLSR(n8 x) -> n8 {
  CCR.C = x & 1;
  x = x >> 1;
  CCR.Z = x == 0;
  CCR.N = 0;
  return x;
}

auto M68HC05::algorithmNEG(n8 x) -> n8 {
  x = -x;
  CCR.C = 0;  //todo
  CCR.Z = x == 0;
  CCR.N = x & 0x80;
  return x;
}

auto M68HC05::algorithmOR(n8 x, n8 y) -> n8 {
  x = x | y;
  CCR.Z = x == 0;
  CCR.N = x & 0x80;
  return x;
}

auto M68HC05::algorithmROL(n8 x) -> n8 {
  b1 c = CCR.C;
  CCR.C = x >> 1;
  x = x << 1 | c;
  CCR.Z = x == 0;
  CCR.N = x & 0x80;
  return x;
}

auto M68HC05::algorithmROR(n8 x) -> n8 {
  b1 c = CCR.C;
  CCR.C = x & 1;
  x = c << 7 | x >> 1;
  CCR.Z = x == 0;
  CCR.N = x & 0x80;
  return x;
}

auto M68HC05::algorithmSBC(n8 x, n8 y) -> n8 {
  n8 z = x - y - !CCR.C;
  n8 c = x ^ ~y ^ z;
  n8 o = (x ^ z) & (y ^ x);
  CCR.C = (c ^ o) & 0x80;
  CCR.Z = z == 0;
  CCR.N = z & 0x80;
  CCR.H = c & 0x10;
  return z;
}

auto M68HC05::algorithmSUB(n8 x, n8 y) -> n8 {
  n8 z = x - y;
  n8 c = x ^ ~y ^ z;
  n8 o = (x ^ z) & (y ^ x);
  CCR.C = (c ^ o) & 0x80;
  CCR.Z = z == 0;
  CCR.N = z & 0x80;
  CCR.C = c & 0x10;
  return z;
}

auto M68HC05::algorithmTST(n8 x) -> n8 {
  CCR.Z = x == 0;
  CCR.N = x & 0x80;
  return x;
}
