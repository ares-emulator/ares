//(0 = odd, 1 = even) number of bits set in value
auto V30MZ::parity(n8 value) const -> bool {
  value ^= value >> 4;
  value ^= value >> 2;
  value ^= value >> 1;
  return !(value & 1);
}

#define bits (size == Byte ? 8 : 16)
#define mask (size == Byte ? 0xff : 0xffff)
#define sign (size == Byte ? 0x80 : 0x8000)

auto V30MZ::ADC(Size size, n16 x, n16 y) -> n16 {
  return ADD(size, x, y + r.f.c);
}

auto V30MZ::ADD(Size size, n16 x, n16 y) -> n16 {
  n16 result = (x + y) & mask;
  r.f.c = x + y > mask;
  r.f.p = parity(result);
  r.f.h = (n4)x + (n4)y >= 16;
  r.f.z = result == 0;
  r.f.s = result & sign;
  r.f.v = (result ^ x) & (result ^ y) & sign;
  return result;
}

auto V30MZ::AND(Size size, n16 x, n16 y) -> n16 {
  n16 result = (x & y) & mask;
  r.f.c = 0;
  r.f.p = parity(result);
  r.f.h = 0;
  r.f.z = result == 0;
  r.f.s = result & sign;
  r.f.v = 0;
  return result;
}

auto V30MZ::DEC(Size size, n16 x) -> n16 {
  n16 result = (x - 1) & mask;
  r.f.p = parity(result);
  r.f.h = (x & 0x0f) == 0;
  r.f.z = result == 0;
  r.f.s = result & sign;
  r.f.v = result == sign - 1;
  return result;
}

auto V30MZ::DIV(Size size, n32 x, n32 y) -> n32 {
  if(y == 0) return interrupt(0), 0;
  n32 quotient = x / y;
  n32 remainder = x % y;
  return (remainder & mask) << bits | (quotient & mask);
}

auto V30MZ::DIVI(Size size, i32 x, i32 y) -> n32 {
  if(y == 0) return interrupt(0), 0;
  x = size == Byte ? (s8)x : (s16)x;
  y = size == Byte ? (s8)y : (s16)y;
  n32 quotient = x / y;
  n32 remainder = x % y;
  return (remainder & mask) << bits | (quotient & mask);
}

auto V30MZ::INC(Size size, n16 x) -> n16 {
  n16 result = (x + 1) & mask;
  r.f.p = parity(result);
  r.f.h = (x & 0x0f) == 0x0f;
  r.f.z = result == 0;
  r.f.s = result & sign;
  r.f.v = result == sign;
  return result;
}

auto V30MZ::MUL(Size size, n16 x, n16 y) -> n32 {
  n32 result = x * y;
  r.f.c = result >> bits;
  r.f.v = result >> bits;
  return result;
}

auto V30MZ::MULI(Size size, i16 x, i16 y) -> n32 {
  x = size == Byte ? (s8)x : (s16)x;
  y = size == Byte ? (s8)y : (s16)y;
  n32 result = x * y;
  r.f.c = result >> bits;
  r.f.v = result >> bits;
  return result;
}

auto V30MZ::NEG(Size size, n16 x) -> n16 {
  n16 result = (-x) & mask;
  r.f.c = x;
  r.f.p = parity(result);
  r.f.h = x & 0x0f;
  r.f.z = result == 0;
  r.f.s = result & sign;
  r.f.v = x == sign;
  return result;
}

auto V30MZ::NOT(Size size, n16 x) -> n16 {
  n16 result = (~x) & mask;
  return result;
}

auto V30MZ::OR(Size size, n16 x, n16 y) -> n16 {
  n16 result = (x | y) & mask;
  r.f.c = 0;
  r.f.p = parity(result);
  r.f.h = 0;
  r.f.z = result == 0;
  r.f.s = result & sign;
  r.f.v = 0;
  return result;
}

auto V30MZ::RCL(Size size, n16 x, n5 y) -> n16 {
  n16 result = x;
  for(u32 n = 0; n < y; n++) {
    bool carry = result & sign;
    result = (result << 1) | r.f.c;
    r.f.c = carry;
  }
  r.f.v = (x ^ result) & sign;
  return result & mask;
}

auto V30MZ::RCR(Size size, n16 x, n5 y) -> n16 {
  n16 result = x;
  for(u32 n = 0; n < y; n++) {
    bool carry = result & 1;
    result = (r.f.c ? sign : 0) | (result >> 1);
    r.f.c = carry;
  }
  r.f.v = (x ^ result) & sign;
  return result & mask;
}

auto V30MZ::ROL(Size size, n16 x, n4 y) -> n16 {
  r.f.c = (x << y) & (1 << bits);
  n16 result = ((x << y) | (x >> (bits - y))) & mask;
  r.f.v = (x ^ result) & sign;
  return result;
}

auto V30MZ::ROR(Size size, n16 x, n4 y) -> n16 {
  r.f.c = (x >> (y - 1)) & 1;
  n16 result = ((x >> y) | (x << (bits - y))) & mask;
  r.f.v = (x ^ result) & sign;
  return result;
}

auto V30MZ::SAL(Size size, n16 x, n5 y) -> n16 {
  r.f.c = (x << y) & (1 << bits);
  n16 result = (x << y) & mask;
  r.f.p = parity(result);
  r.f.z = result == 0;
  r.f.s = result & sign;
  r.f.v = 0;
  return result;
}

auto V30MZ::SAR(Size size, n16 x, n5 y) -> n16 {
  if(y & 16) {
    r.f.c = x & sign;
    return 0 - r.f.c;
  }
  r.f.c = (x >> (y - 1)) & 1;
  n16 result = (x >> y) & mask;
  if(x & sign) result |= mask << (bits - y);
  r.f.p = parity(result);
  r.f.z = result == 0;
  r.f.s = result & sign;
  r.f.v = 0;
  return result;
}

auto V30MZ::SBB(Size size, n16 x, n16 y) -> n16 {
  return SUB(size, x, y + r.f.c);
}

auto V30MZ::SHL(Size size, n16 x, n5 y) -> n16 {
  r.f.c = (x << y) & (1 << bits);
  n16 result = (x << y) & mask;
  r.f.p = parity(result);
  r.f.z = result == 0;
  r.f.s = result & sign;
  r.f.v = (x ^ result) & sign;
  return result;
}

auto V30MZ::SHR(Size size, n16 x, n5 y) -> n16 {
  r.f.c = (x >> (y - 1)) & 1;
  n16 result = (x >> y) & mask;
  r.f.p = parity(result);
  r.f.z = result == 0;
  r.f.s = result & sign;
  r.f.v = (x ^ result) & sign;
  return result;
}

auto V30MZ::SUB(Size size, n16 x, n16 y) -> n16 {
  n16 result = (x - y) & mask;
  r.f.c = y > x;
  r.f.p = parity(result);
  r.f.h = (n4)y > (n4)x;
  r.f.z = result == 0;
  r.f.s = result & sign;
  r.f.v = (x ^ y) & (x ^ result) & sign;
  return result;
}

auto V30MZ::XOR(Size size, n16 x, n16 y) -> n16 {
  n16 result = (x ^ y) & mask;
  r.f.c = 0;
  r.f.p = parity(result);
  r.f.h = 0;
  r.f.z = result == 0;
  r.f.s = result & sign;
  r.f.v = 0;
  return result;
}

#undef mask
#undef sign
