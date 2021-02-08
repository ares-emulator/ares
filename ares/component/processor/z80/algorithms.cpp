auto Z80::ADD(n8 x, n8 y, bool c) -> n8 {
  n9 z = x + y + c;

  CF = z.bit(8);
  NF = 0;
  VF = n8(~(x ^ y) & (x ^ z)).bit(7);
  XF = z.bit(3);
  HF = n8(x ^ y ^ z).bit(4);
  YF = z.bit(5);
  ZF = n8(z) == 0;
  SF = z.bit(7);

  return z;
}

auto Z80::AND(n8 x, n8 y) -> n8 {
  n8 z = x & y;

  CF = 0;
  NF = 0;
  PF = parity(z);
  XF = z.bit(3);
  HF = 1;
  YF = z.bit(5);
  ZF = z == 0;
  SF = z.bit(7);

  return z;
}

auto Z80::BIT(n3 bit, n8 x) -> n8 {
  n8 z = x & 1 << bit;

  NF = 0;
  PF = parity(z);
  XF = x.bit(3);
  HF = 1;
  YF = x.bit(5);
  ZF = z == 0;
  SF = z.bit(7);

  return x;
}

auto Z80::CP(n8 x, n8 y) -> void {
  n9 z = x - y;

  CF = z.bit(8);
  NF = 1;
  VF = n8((x ^ y) & (x ^ z)).bit(7);
  XF = y.bit(3);
  HF = n8(x ^ y ^ z).bit(4);
  YF = y.bit(5);
  ZF = n8(z) == 0;
  SF = z.bit(7);
}

auto Z80::DEC(n8 x) -> n8 {
  n8 z = x - 1;

  NF = 1;
  VF = z == 0x7f;
  XF = z.bit(3);
  HF = z.bit(0,3) == 0x0f;
  YF = z.bit(5);
  ZF = z == 0;
  SF = z.bit(7);

  return z;
}

auto Z80::IN(n8 x) -> n8 {
  NF = 0;
  PF = parity(x);
  XF = x.bit(3);
  HF = 0;
  YF = x.bit(5);
  ZF = x == 0;
  SF = x.bit(7);

  return x;
}

auto Z80::INC(n8 x) -> n8 {
  n8 z = x + 1;

  NF = 0;
  VF = z == 0x80;
  XF = z.bit(3);
  HF = z.bit(0,3) == 0x00;
  YF = z.bit(5);
  ZF = z == 0;
  SF = z.bit(7);

  return z;
}

auto Z80::OR(n8 x, n8 y) -> n8 {
  n8 z = x | y;

  CF = 0;
  NF = 0;
  PF = parity(z);
  XF = z.bit(3);
  HF = 0;
  YF = z.bit(5);
  ZF = z == 0;
  SF = z.bit(7);

  return z;
}

auto Z80::RES(n3 bit, n8 x) -> n8 {
  x &= ~(1 << bit);
  return x;
}

auto Z80::RL(n8 x) -> n8 {
  bool c = x.bit(7);
  x = x << 1 | CF;

  CF = c;
  NF = 0;
  PF = parity(x);
  XF = x.bit(3);
  HF = 0;
  YF = x.bit(5);
  ZF = x == 0;
  SF = x.bit(7);

  return x;
}

auto Z80::RLC(n8 x) -> n8 {
  x = x << 1 | x >> 7;

  CF = x.bit(0);
  NF = 0;
  PF = parity(x);
  XF = x.bit(3);
  HF = 0;
  YF = x.bit(5);
  ZF = x == 0;
  SF = x.bit(7);

  return x;
}

auto Z80::RR(n8 x) -> n8 {
  bool c = x.bit(0);
  x = x >> 1 | CF << 7;

  CF = c;
  NF = 0;
  PF = parity(x);
  XF = x.bit(3);
  HF = 0;
  YF = x.bit(5);
  ZF = x == 0;
  SF = x.bit(7);

  return x;
}

auto Z80::RRC(n8 x) -> n8 {
  x = x >> 1 | x << 7;

  CF = x.bit(7);
  NF = 0;
  PF = parity(x);
  XF = x.bit(3);
  HF = 0;
  YF = x.bit(5);
  ZF = x == 0;
  SF = x.bit(7);

  return x;
}

auto Z80::SET(n3 bit, n8 x) -> n8 {
  x |= (1 << bit);
  return x;
}

auto Z80::SLA(n8 x) -> n8 {
  bool c = x.bit(7);
  x = x << 1;

  CF = c;
  NF = 0;
  PF = parity(x);
  XF = x.bit(3);
  HF = 0;
  YF = x.bit(5);
  ZF = x == 0;
  SF = x.bit(7);

  return x;
}

auto Z80::SLL(n8 x) -> n8 {
  bool c = x.bit(7);
  x = x << 1 | 1;

  CF = c;
  NF = 0;
  PF = parity(x);
  XF = x.bit(3);
  HF = 0;
  YF = x.bit(5);
  ZF = x == 0;
  SF = x.bit(7);

  return x;
}

auto Z80::SRA(n8 x) -> n8 {
  bool c = x.bit(0);
  x = (i8)x >> 1;

  CF = c;
  NF = 0;
  PF = parity(x);
  XF = x.bit(3);
  HF = 0;
  YF = x.bit(5);
  ZF = x == 0;
  SF = x.bit(7);

  return x;
}

auto Z80::SRL(n8 x) -> n8 {
  bool c = x.bit(0);
  x = x >> 1;

  CF = c;
  NF = 0;
  PF = parity(x);
  XF = x.bit(3);
  HF = 0;
  YF = x.bit(5);
  ZF = x == 0;
  SF = x.bit(7);

  return x;
}

auto Z80::SUB(n8 x, n8 y, bool c) -> n8 {
  n9 z = x - y - c;

  CF = z.bit(8);
  NF = 1;
  VF = n8((x ^ y) & (x ^ z)).bit(7);
  XF = z.bit(3);
  HF = n8(x ^ y ^ z).bit(4);
  YF = z.bit(5);
  ZF = n8(z) == 0;
  SF = z.bit(7);

  return z;
}

auto Z80::XOR(n8 x, n8 y) -> n8 {
  n8 z = x ^ y;

  CF = 0;
  NF = 0;
  PF = parity(z);
  XF = z.bit(3);
  HF = 0;
  YF = z.bit(5);
  ZF = z == 0;
  SF = z.bit(7);

  return z;
}
