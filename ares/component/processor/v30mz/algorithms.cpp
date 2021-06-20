//(0 = odd, 1 = even) number of bits set in value
auto V30MZ::parity(u8 value) const -> bool {
  value ^= value >> 4;
  value ^= value >> 2;
  value ^= value >> 1;
  return !(value & 1);
}

#define bits (size == Byte ? 8 : 16)
#define mask (size == Byte ? 0xff : 0xffff)
#define sign (size == Byte ? 0x80 : 0x8000)

template<u32 size> auto V30MZ::ADC(u16 x, u16 y) -> u16 {
  return ADD<size>(x, y + PSW.CY);
}

template<u32 size> auto V30MZ::ADD(u16 x, u16 y) -> u16 {
  u16 result = (x + y) & mask;
  PSW.CY = x + y > mask;
  PSW.P  = parity(result);
  PSW.AC = (u4)x + (u4)y >= 16;
  PSW.Z  = result == 0;
  PSW.S  = result & sign;
  PSW.V  = (result ^ x) & (result ^ y) & sign;
  return result;
}

template<u32 size> auto V30MZ::AND(u16 x, u16 y) -> u16 {
  u16 result = (x & y) & mask;
  PSW.CY = 0;
  PSW.P  = parity(result);
  PSW.AC  = 0;
  PSW.Z  = result == 0;
  PSW.S  = result & sign;
  PSW.V  = 0;
  return result;
}

template<u32 size> auto V30MZ::DEC(u16 x) -> u16 {
  u16 result = (x - 1) & mask;
  PSW.P  = parity(result);
  PSW.AC = (x & 0x0f) == 0;
  PSW.Z  = result == 0;
  PSW.S  = result & sign;
  PSW.V  = result == sign - 1;
  return result;
}

template<u32 size> auto V30MZ::DIVI(s32 x, s32 y) -> u32 {
  if(y == 0) return interrupt(0), x;
  x = size == Byte ? (s16)x : (s32)x;
  y = size == Byte ? ( s8)y : (s16)y;
  s32 quotient  = x / y;
  s32 remainder = x % y;
  if(quotient > mask >> 1 || quotient < -sign) return interrupt(0), x;
  return (remainder & mask) << bits | (quotient & mask);
}

template<u32 size> auto V30MZ::DIVU(u32 x, u32 y) -> u32 {
  if(y == 0) return interrupt(0), x;
  u32 quotient  = x / y;
  u32 remainder = x % y;
  if(quotient > mask) return interrupt(0), x;
  return (remainder & mask) << bits | (quotient & mask);
}

template<u32 size> auto V30MZ::INC(u16 x) -> u16 {
  u16 result = (x + 1) & mask;
  PSW.P  = parity(result);
  PSW.AC = (x & 0x0f) == 0x0f;
  PSW.Z  = result == 0;
  PSW.S  = result & sign;
  PSW.V  = result == sign;
  return result;
}

template<u32 size> auto V30MZ::MULI(s16 x, s16 y) -> u32 {
  x = size == Byte ? (s8)x : (s16)x;
  y = size == Byte ? (s8)y : (s16)y;
  u32 result = x * y;
  PSW.CY = result >> bits;
  PSW.V  = result >> bits;
  return result;
}

template<u32 size> auto V30MZ::MULU(u16 x, u16 y) -> u32 {
  u32 result = x * y;
  PSW.CY = result >> bits;
  PSW.V  = result >> bits;
  return result;
}

template<u32 size> auto V30MZ::NEG(u16 x) -> u16 {
  u16 result = (-x) & mask;
  PSW.CY = x;
  PSW.P  = parity(result);
  PSW.AC = x & 0x0f;
  PSW.Z  = result == 0;
  PSW.S  = result & sign;
  PSW.V  = x == sign;
  return result;
}

template<u32 size> auto V30MZ::NOT(u16 x) -> u16 {
  u16 result = (~x) & mask;
  return result;
}

template<u32 size> auto V30MZ::OR(u16 x, u16 y) -> u16 {
  u16 result = (x | y) & mask;
  PSW.CY = 0;
  PSW.P  = parity(result);
  PSW.AC = 0;
  PSW.Z  = result == 0;
  PSW.S  = result & sign;
  PSW.V  = 0;
  return result;
}

template<u32 size> auto V30MZ::RCL(u16 x, u5 y) -> u16 {
  u16 result = x;
  for(u32 n = 0; n < y; n++) {
    bool carry = result & sign;
    result = (result << 1) | PSW.CY;
    PSW.CY = carry;
  }
  PSW.V = (x ^ result) & sign;
  return result & mask;
}

template<u32 size> auto V30MZ::RCR(u16 x, u5 y) -> u16 {
  u16 result = x;
  for(u32 n = 0; n < y; n++) {
    bool carry = result & 1;
    result = (PSW.CY ? sign : 0) | (result >> 1);
    PSW.CY = carry;
  }
  PSW.V = (x ^ result) & sign;
  return result & mask;
}

template<u32 size> auto V30MZ::ROL(u16 x, u4 y) -> u16 {
  PSW.CY = (x << y) & (1 << bits);
  u16 result = ((x << y) | (x >> (bits - y))) & mask;
  PSW.V = (x ^ result) & sign;
  return result;
}

template<u32 size> auto V30MZ::ROR(u16 x, u4 y) -> u16 {
  PSW.CY = (x >> (y - 1)) & 1;
  u16 result = ((x >> y) | (x << (bits - y))) & mask;
  PSW.V = (x ^ result) & sign;
  return result;
}

template<u32 size> auto V30MZ::SAL(u16 x, u5 y) -> u16 {
  PSW.CY = (x << y) & (1 << bits);
  u16 result = (x << y) & mask;
  PSW.P = parity(result);
  PSW.Z = result == 0;
  PSW.S = result & sign;
  PSW.V = 0;
  return result;
}

template<u32 size> auto V30MZ::SAR(u16 x, u5 y) -> u16 {
  if(y & 16) {
    PSW.CY = x & sign;
    return 0 - PSW.CY;
  }
  PSW.CY = (x >> (y - 1)) & 1;
  u16 result = (x >> y) & mask;
  if(x & sign) result |= mask << (bits - y);
  PSW.P = parity(result);
  PSW.Z = result == 0;
  PSW.S = result & sign;
  PSW.V = 0;
  return result;
}

template<u32 size> auto V30MZ::SBB(u16 x, u16 y) -> u16 {
  return SUB<size>(x, y + PSW.CY);
}

template<u32 size> auto V30MZ::SHL(u16 x, u5 y) -> u16 {
  PSW.CY = (x << y) & (1 << bits);
  u16 result = (x << y) & mask;
  PSW.P = parity(result);
  PSW.Z = result == 0;
  PSW.S = result & sign;
  PSW.V = (x ^ result) & sign;
  return result;
}

template<u32 size> auto V30MZ::SHR(u16 x, u5 y) -> u16 {
  PSW.CY = (x >> (y - 1)) & 1;
  u16 result = (x >> y) & mask;
  PSW.P = parity(result);
  PSW.Z = result == 0;
  PSW.S = result & sign;
  PSW.V = (x ^ result) & sign;
  return result;
}

template<u32 size> auto V30MZ::SUB(u16 x, u16 y) -> u16 {
  u16 result = (x - y) & mask;
  PSW.CY = y > x;
  PSW.P  = parity(result);
  PSW.AC = (u4)y > (u4)x;
  PSW.Z  = result == 0;
  PSW.S  = result & sign;
  PSW.V  = (x ^ y) & (x ^ result) & sign;
  return result;
}

template<u32 size> auto V30MZ::XOR(u16 x, u16 y) -> u16 {
  u16 result = (x ^ y) & mask;
  PSW.CY = 0;
  PSW.P  = parity(result);
  PSW.AC = 0;
  PSW.Z  = result == 0;
  PSW.S  = result & sign;
  PSW.V  = 0;
  return result;
}

#undef mask
#undef sign
