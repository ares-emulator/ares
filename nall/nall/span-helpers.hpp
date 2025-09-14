#pragma once

#include <nall/traits.hpp>
#include <nall/range.hpp>
#include <nall/stdint.hpp>
#include <span>

namespace nall {

template<typename T> inline auto consume_front(std::span<const T>& s) -> T { auto v = s.front(); s = s.subspan(1); return v; }
template<typename T> inline auto write_front(std::span<T>& s, const T& v) -> void { s.front() = v; s = s.subspan(1); }

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

template<typename T> inline auto at_or(std::span<const T> s, u32 i, T fallback = {}) -> T {
  return i < s.size() ? s[i] : fallback;
}

// Helper function to create span from nall::Natural pointer and size
template<u32 N> 
inline auto make_span(const Natural<N>* ptr, size_t size) -> std::span<const u8> {
  return std::span<const u8>(reinterpret_cast<const u8*>(ptr), size);
}

template<u32 N> 
inline auto make_span(Natural<N>* ptr, size_t size) -> std::span<u8> {
  return std::span<u8>(reinterpret_cast<u8*>(ptr), size);
}

// Special version for Natural<8> that preserves the type
inline auto make_span(Natural<8>* ptr, size_t size) -> std::span<Natural<8>> {
  return std::span<Natural<8>>(ptr, size);
}

inline auto make_span(const Natural<8>* ptr, size_t size) -> std::span<const Natural<8>> {
  return std::span<const Natural<8>>(ptr, size);
}

}


