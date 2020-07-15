#pragma once

#include <nall/stdint.hpp>

namespace nall {

template<uint bits, typename T> inline auto uclamp(T x) -> uint64_t {
  enum : uint64_t { b = 1ull << (bits - 1), y = b * 2 - 1 };
  if constexpr(is_unsigned_v<T>) {
    return y + ((x - y) & -(x < y));  //min(x, y);
  }
  if constexpr(is_signed_v<T>) {
    return x < 0 ? 0 : x > y ? y : x;
  }
}

template<uint bits> inline auto uclip(uint64_t x) -> uint64_t {
  enum : uint64_t { b = 1ull << (bits - 1), m = b * 2 - 1 };
  return (x & m);
}

template<uint bits> inline auto sclamp(int64_t x) -> int64_t {
  enum : int64_t { b = 1ull << (bits - 1), m = b - 1 };
  return (x > m) ? m : (x < -b) ? -b : x;
}

template<uint bits> inline auto sclip(int64_t x) -> int64_t {
  enum : uint64_t { b = 1ull << (bits - 1), m = b * 2 - 1 };
  return ((x & m) ^ b) - b;
}

namespace bit {
  constexpr inline auto mask(const char* s, uint64_t sum = 0) -> uint64_t {
    return (
      *s == '0' || *s == '1' ? mask(s + 1, (sum << 1) | 1) :
      *s == ' ' || *s == '_' ? mask(s + 1, sum) :
      *s ? mask(s + 1, sum << 1) :
      sum
    );
  }

  constexpr inline auto test(const char* s, uint64_t sum = 0) -> uint64_t {
    return (
      *s == '0' || *s == '1' ? test(s + 1, (sum << 1) | (*s - '0')) :
      *s == ' ' || *s == '_' ? test(s + 1, sum) :
      *s ? test(s + 1, sum << 1) :
      sum
    );
  }

  //lowest(0b1110) == 0b0010
  constexpr inline auto lowest(const uint64_t x) -> uint64_t {
    return x & -x;
  }

  //clear_lowest(0b1110) == 0b1100
  constexpr inline auto clearLowest(const uint64_t x) -> uint64_t {
    return x & (x - 1);
  }

  //set_lowest(0b0101) == 0b0111
  constexpr inline auto setLowest(const uint64_t x) -> uint64_t {
    return x | (x + 1);
  }

  //count number of bits set in a byte
  constexpr inline auto count(uint64_t x) -> uint {
    uint count = 0;
    while(x) x &= x - 1, count++;  //clear the least significant bit
    return count;
  }

  //return index of the first bit set (or zero of no bits are set)
  //first(0b1000) == 3
  constexpr inline auto first(uint64_t x) -> uint {
    uint first = 0;
    while(x) { if(x & 1) break; x >>= 1; first++; }
    return first;
  }

  //round up to next highest single bit:
  //round(15) == 16, round(16) == 16, round(17) == 32
  constexpr inline auto round(uint64_t x) -> uint64_t {
    if((x & (x - 1)) == 0) return x;
    while(x & (x - 1)) x &= x - 1;
    return x << 1;
  }
}

}
