#include <ares/ares.hpp>
#include "armv6m.hpp"

namespace ares {

#include "decoder.cpp"
#include "disassembler.cpp"
#include "instruction.cpp"
#include "serialization.cpp"

auto ARMv6M::power() -> void {
  processor = {};
  opcode = 0;
  instructionAddress = 0;
}

auto ARMv6M::load(u32 mode, u32 address, n32& data) -> Fault {
  auto size = mode & Byte ? 1u : mode & Half ? 2u : 4u;
  if(address & (size - 1)) return Fault::Alignment;
  n32 value = 0;
  if(auto error = get(mode | Load, address, value); error != Fault::None) return error;
  data = value;
  return Fault::None;
}

auto ARMv6M::store(u32 mode, u32 address, n32 data) -> Fault {
  auto size = mode & Byte ? 1u : mode & Half ? 2u : 4u;
  if(address & (size - 1)) return Fault::Alignment;
  if(auto error = set(mode | Store, address, data); error != Fault::None) return error;
  return Fault::None;
}

auto ARMv6M::branch(u32 address) -> Fault {
  if(address == 0xffff'ffff) return Fault::Return;
  if(!(address & 1)) return Fault::Alignment;
  r(15) = address & ~1u;
  return Fault::None;
}

auto ARMv6M::condition(u32 code) const -> bool {
  switch(code & 15) {
  case 0x0: return psr().z;
  case 0x1: return !psr().z;
  case 0x2: return psr().c;
  case 0x3: return !psr().c;
  case 0x4: return psr().n;
  case 0x5: return !psr().n;
  case 0x6: return psr().v;
  case 0x7: return !psr().v;
  case 0x8: return psr().c && !psr().z;
  case 0x9: return !psr().c || psr().z;
  case 0xa: return psr().n == psr().v;
  case 0xb: return psr().n != psr().v;
  case 0xc: return !psr().z && psr().n == psr().v;
  case 0xd: return psr().z || psr().n != psr().v;
  case 0xe: return true;
  default:  return false;
  }
}

auto ARMv6M::setNZ(u32 value) -> void {
  psr().n = value >> 31;
  psr().z = value == 0;
}

auto ARMv6M::add(u32 lhs, u32 rhs, u32 carryIn, bool flags) -> u32 {
  u64 wide = (u64)lhs + rhs + carryIn;
  auto result = (u32)wide;
  if(flags) {
    setNZ(result);
    psr().c = wide >> 32;
    auto signedWide = (i64)(i32)lhs + (i64)(i32)rhs + carryIn;
    psr().v = signedWide < -0x8000'0000ll || signedWide > 0x7fff'ffffll;
  }
  return result;
}

auto ARMv6M::sub(u32 lhs, u32 rhs, u32 carryIn, bool flags) -> u32 {
  auto result = lhs - rhs - (1 - carryIn);
  if(flags) {
    setNZ(result);
    psr().c = (u64)lhs >= (u64)rhs + (1 - carryIn);
    auto signedWide = (i64)(i32)lhs - (i64)(i32)rhs - (1 - carryIn);
    psr().v = signedWide < -0x8000'0000ll || signedWide > 0x7fff'ffffll;
  }
  return result;
}

auto ARMv6M::shiftLSL(u32 value, u32 amount, bool flags) -> u32 {
  if(!amount) { if(flags) setNZ(value); return value; }
  if(amount < 32) { if(flags) psr().c = value >> (32 - amount) & 1; value <<= amount; }
  else { if(flags) psr().c = amount == 32 ? value & 1 : 0; value = 0; }
  if(flags) setNZ(value);
  return value;
}

auto ARMv6M::shiftLSR(u32 value, u32 amount, bool flags) -> u32 {
  if(!amount) amount = 32;
  if(amount < 32) { if(flags) psr().c = value >> (amount - 1) & 1; value >>= amount; }
  else { if(flags) psr().c = amount == 32 ? value >> 31 : 0; value = 0; }
  if(flags) setNZ(value);
  return value;
}

auto ARMv6M::shiftASR(u32 value, u32 amount, bool flags) -> u32 {
  if(!amount) amount = 32;
  if(amount < 32) {
    if(flags) psr().c = value >> (amount - 1) & 1;
    value = (u32)((i32)value >> amount);
  } else {
    if(flags) psr().c = value >> 31;
    value = (i32)value < 0 ? ~0u : 0u;
  }
  if(flags) setNZ(value);
  return value;
}

auto ARMv6M::shiftROR(u32 value, u32 amount, bool flags) -> u32 {
  if(!amount) { if(flags) setNZ(value); return value; }
  amount &= 31;
  if(!amount) { if(flags) psr().c = value >> 31; }
  else {
    value = value >> amount | value << (32 - amount);
    if(flags) psr().c = value >> 31;
  }
  if(flags) setNZ(value);
  return value;
}

}
