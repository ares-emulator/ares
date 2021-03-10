auto SSP1601::updateZN() -> void {
  Z = A == 0;
  N = A >> 31;
}

auto SSP1601::updateLZVN() -> void {
  L = 0;
  Z = A == 0;
  V = 0;
  N = A >> 31;
}

auto SSP1601::test(u16 op) -> bool {
  bool c = n1(op >> 8);
  switch(n4(op >> 4)) {
  case 0: return 1;
  case 5: return Z == c;
  case 7: return N == c;
  }
  return 0;  //unsupported condition
}

auto SSP1601::sub(u32 v) -> void {
  A -= v;
  updateLZVN();
}

auto SSP1601::cmp(u32 v) -> void {
  u32 a = A - v;
  L = 0;
  Z = a == 0;
  V = 0;
  N = a >> 31;
}

auto SSP1601::add(u32 v) -> void {
  A += v;
  updateLZVN();
}

auto SSP1601::and(u32 v) -> void {
  A &= v;
  updateZN();
}

auto SSP1601::or(u32 v) -> void {
  A |= v;
  updateZN();
}

auto SSP1601::eor(u32 v) -> void {
  A ^= v;
  updateZN();
}

auto SSP1601::shr() -> void {
  A = (s32)A >> 1;
  updateZN();
}

auto SSP1601::shl() -> void {
  A <<= 1;
  updateZN();
}

auto SSP1601::neg() -> void {
  A = -(s32)A;
  updateZN();
}

auto SSP1601::abs() -> void {
  if((s32)A < 0) A = -(s32)A;
  updateZN();
}
