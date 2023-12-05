auto I8080::ADD(n8 x, n8 y, bool c) -> n8 {
  n9 z = x + y + c;

  CF = z.bit(8);
  PF = parity(z);
  HF = n8(x ^ y ^ z).bit(4);
  ZF = n8(z) == 0;
  SF = z.bit(7);

  return z;
}

auto I8080::AND(n8 x, n8 y) -> n8 {
  n8 z = x & y;

  CF = 0;
  HF = n8(x | y).bit(3);
  PF = parity(z);
  ZF = z == 0;
  SF = z.bit(7);

  return z;
}

auto I8080::CP(n8 x, n8 y) -> void {
  SUB(x, y, 0);
}

auto I8080::DEC(n8 x) -> n8 {
  n8 z = x - 1;

  PF = parity(z);
  HF = x.bit(0,3) != 0x00;
  ZF = z == 0;
  SF = z.bit(7);

  return z;
}

auto I8080::INC(n8 x) -> n8 {
  n8 z = x + 1;

  PF = parity(z);
  HF = x.bit(0,3) == 0x0f;
  ZF = z == 0;
  SF = z.bit(7);

  return z;
}

auto I8080::OR(n8 x, n8 y) -> n8 {
  n8 z = x | y;

  CF = 0;
  PF = parity(z);
  HF = 0;
  ZF = z == 0;
  SF = z.bit(7);

  return z;
}

auto I8080::SUB(n8 x, n8 y, bool c) -> n8 {
  n9 z = x + ~y + !c; // internally implemented as negated ADD

  CF = z.bit(8);
  PF = parity(z);
  HF = n8(x ^ ~y ^ z).bit(4);
  ZF = n8(z) == 0;
  SF = z.bit(7);

  return z;
}

auto I8080::XOR(n8 x, n8 y) -> n8 {
  n8 z = x ^ y;

  CF = 0;
  HF = 0;
  PF = parity(z);
  ZF = z == 0;
  SF = z.bit(7);

  return z;
}
