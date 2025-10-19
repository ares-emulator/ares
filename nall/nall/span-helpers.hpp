#pragma once

#include <nall/traits.hpp>
#include <nall/range.hpp>
#include <nall/stdint.hpp>
#include <span>

namespace nall {

template<typename T> inline auto consume_front(std::span<const T>& s) -> T {
  auto v = s.front();
  s = s.subspan(1);
  return v;
}

template<typename U> inline auto readm(std::span<const u8>& s, u32 size) -> U {
  U v = 0;
  for(u32 i : range(size)) { v = (U)(v << 8) | s.front(); s = s.subspan(1); }
  return v;
}

template<typename U> inline auto writel(std::span<u8>& s, U v, u32 size) -> void {
  for(u32 i : range(size)) { s.front() = (u8)(v >> (i * 8)); s = s.subspan(1); }
}

template<typename U> inline auto writem(std::span<u8>& s, U v, u32 size) -> void {
  for(u32 i : range(size)) { s.front() = (u8)(v >> ((size - 1 - i) * 8)); s = s.subspan(1); }
}

}
