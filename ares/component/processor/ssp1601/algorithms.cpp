auto SSP1601::test(u16 op) -> bool {
  auto flag = n1(op >> 8);
  switch(n3(op >> 4)) {
  case 0: return 0 == flag;  //always/never
  case 1: return 0;  //unsupported
  case 2: return USR0 == flag;
  case 3: return USR1 == flag;
  case 4: return C == flag;
  case 5: return Z == flag;
  case 6: return V == flag;
  case 7: return N == flag;
  }
  unreachable;
}

auto SSP1601::add(u32 source) -> void {
  u32 result   = A + source;
  u32 carries  = A ^ source ^ result;
  u32 overflow = (A ^ result) & (source ^ result);
  C = (carries ^ overflow) >> 31;
  V = overflow >> 31;
  A = (!OP || !V) ? result : 0x7fff'ffff;
  Z = A == 0;
  N = A >> 31;
}

auto SSP1601::sub(u32 source) -> void {
  u32 result   = A - source;
  u32 carries  = A ^ source ^ result;
  u32 overflow = (A ^ result) & (source ^ A);
  C = (carries ^ overflow) >> 31;
  V = overflow >> 31;
  A = (!OP || !V) ? result : 0x8000'0000;
  Z = A == 0;
  N = A >> 31;
}

auto SSP1601::cmp(u32 source) -> void {
  u32 result   = A - source;
  u32 carries  = A ^ source ^ result;
  u32 overflow = (A ^ result) & (source ^ A);
  C = (carries ^ overflow) >> 31;
  V = overflow >> 31;
  Z = result == 0;
  N = result >> 31;
}

auto SSP1601::and(u32 v) -> void {
  A &= v;
  Z = A == 0;
  N = A >> 31;
}

auto SSP1601::or(u32 v) -> void {
  A |= v;
  Z = A == 0;
  N = A >> 31;
}

auto SSP1601::eor(u32 v) -> void {
  A ^= v;
  Z = A == 0;
  N = A >> 31;
}

auto SSP1601::shr() -> void {
  A = (s32)A >> 1;
  Z = A == 0;
  N = A >> 31;
}

auto SSP1601::shl() -> void {
  A <<= 1;
  Z = A == 0;
  N = A >> 31;
}

auto SSP1601::neg() -> void {
  A = -(s32)A;
  Z = A == 0;
  N = A >> 31;
}

auto SSP1601::abs() -> void {
  if((s32)A < 0) A = -(s32)A;
  Z = A == 0;
  N = A >> 31;
}
