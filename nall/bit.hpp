#pragma once

#include <nall/stdint.hpp>

namespace nall {

template<u32 bits, typename T> inline auto uclamp(T x) -> u64 {
  enum : u64 { b = 1ull << (bits - 1), y = b * 2 - 1 };
  if constexpr(is_unsigned_v<T>) {
    return y + ((x - y) & -(x < y));  //min(x, y);
  }
  if constexpr(is_signed_v<T>) {
    return x < 0 ? 0 : x > y ? y : x;
  }
}

template<u32 bits> inline auto uclip(u64 x) -> u64 {
  enum : u64 { b = 1ull << (bits - 1), m = b * 2 - 1 };
  return (x & m);
}

template<u32 bits> inline auto sclamp(s64 x) -> s64 {
  enum : s64 { b = 1ull << (bits - 1), m = b - 1 };
  return (x > m) ? m : (x < -b) ? -b : x;
}

template<u32 bits> inline auto sclip(s64 x) -> s64 {
  enum : u64 { b = 1ull << (bits - 1), m = b * 2 - 1 };
  return ((x & m) ^ b) - b;
}

namespace bit {
  constexpr inline auto mask(const char* s, u64 sum = 0) -> u64 {
    return (
      *s == '0' || *s == '1' ? mask(s + 1, (sum << 1) | 1) :
      *s == ' ' || *s == '_' ? mask(s + 1, sum) :
      *s ? mask(s + 1, sum << 1) :
      sum
    );
  }

  constexpr inline auto test(const char* s, u64 sum = 0) -> u64 {
    return (
      *s == '0' || *s == '1' ? test(s + 1, (sum << 1) | (*s - '0')) :
      *s == ' ' || *s == '_' ? test(s + 1, sum) :
      *s ? test(s + 1, sum << 1) :
      sum
    );
  }

  //lowest(0b1110) == 0b0010
  constexpr inline auto lowest(const u64 x) -> u64 {
    return x & -x;
  }

  //clear_lowest(0b1110) == 0b1100
  constexpr inline auto clearLowest(const u64 x) -> u64 {
    return x & (x - 1);
  }

  //set_lowest(0b0101) == 0b0111
  constexpr inline auto setLowest(const u64 x) -> u64 {
    return x | (x + 1);
  }

  //count number of bits set in a byte
  constexpr inline auto count(u64 x) -> u32 {
    u32 count = 0;
    while(x) x &= x - 1, count++;  //clear the least significant bit
    return count;
  }

  //return index of the first bit set (or zero of no bits are set)
  //first(0b1000) == 3
  constexpr inline auto first(u64 x) -> u32 {
    u32 first = 0;
    while(x) { if(x & 1) break; x >>= 1; first++; }
    return first;
  }

  //round up to next highest single bit:
  //round(15) == 16, round(16) == 16, round(17) == 32
  constexpr inline auto round(u64 x) -> u64 {
    if((x & (x - 1)) == 0) return x;
    while(x & (x - 1)) x &= x - 1;
    return x << 1;
  }
}

}
