#pragma once

#include <nall/iterator.hpp>
#include <nall/range.hpp>
#include <nall/traits.hpp>

namespace nall {

template<typename T> struct array_view {
  using type = array_view;

  array_view() {
    _data = nullptr;
    _size = 0;
  }

  array_view(nullptr_t) {
    _data = nullptr;
    _size = 0;
  }

  array_view(const void* data, uint64_t size) {
    _data = (const T*)data;
    _size = (int)size;
  }

  explicit operator bool() const { return _data && _size > 0; }

  operator const T*() const {
    #ifdef DEBUG
    struct out_of_bounds {};
    if(_size < 0) throw out_of_bounds{};
    #endif
    return _data;
  }

  auto operator++() -> type& { _data++; _size--; return *this; }
  auto operator--() -> type& { _data--; _size++; return *this; }

  auto operator++(int) -> type { auto copy = *this; ++(*this); return copy; }
  auto operator--(int) -> type { auto copy = *this; --(*this); return copy; }

  auto operator-=(int distance) -> type& { _data -= distance; _size += distance; return *this; }
  auto operator+=(int distance) -> type& { _data += distance; _size -= distance; return *this; }

  auto operator[](uint index) const -> const T& {
    #ifdef DEBUG
    struct out_of_bounds {};
    if(index >= _size) throw out_of_bounds{};
    #endif
    return _data[index];
  }

  auto operator()(uint index, const T& fallback = {}) const -> T {
    if(index >= _size) return fallback;
    return _data[index];
  }

  template<typename U = T> auto data() const -> const U* { return (const U*)_data; }
  template<typename U = T> auto size() const -> uint64_t { return _size * sizeof(T) / sizeof(U); }

  auto begin() const -> iterator_const<T> { return {_data, (uint)0}; }
  auto end() const -> iterator_const<T> { return {_data, (uint)_size}; }

  auto rbegin() const -> reverse_iterator_const<T> { return {_data, (uint)_size - 1}; }
  auto rend() const -> reverse_iterator_const<T> { return {_data, (uint)-1}; }

  auto read() -> T {
    auto value = operator[](0);
    _data++;
    _size--;
    return value;
  }

  auto view(uint offset, uint length) const -> type {
    #ifdef DEBUG
    struct out_of_bounds {};
    if(offset + length >= _size) throw out_of_bounds{};
    #endif
    return {_data + offset, length};
  }

  //array_view<uint8_t> specializations
  template<typename U> auto readl(U& value, uint size) -> U;
  template<typename U> auto readm(U& value, uint size) -> U;
  template<typename U> auto readvn(U& value, uint size) -> U;
  template<typename U> auto readvi(U& value, uint size) -> U;

  template<typename U> auto readl(U& value, uint offset, uint size) -> U { return view(offset, size).readl(value, size); }

  template<typename U = uint64_t> auto readl(uint size) -> U { U value; return readl(value, size); }
  template<typename U = uint64_t> auto readm(uint size) -> U { U value; return readm(value, size); }
  template<typename U = uint64_t> auto readvn(uint size) -> U { U value; return readvn(value, size); }
  template<typename U =  int64_t> auto readvi(uint size) -> U { U value; return readvi(value, size); }

  template<typename U = uint64_t> auto readl(uint offset, uint size) -> U { U value; return readl(value, offset, size); }

protected:
  const T* _data;
  int _size;
};

//array_view<uint8_t>

template<> template<typename U> inline auto array_view<uint8_t>::readl(U& value, uint size) -> U {
  value = 0;
  for(uint byte : range(size)) value |= (U)read() << byte * 8;
  return value;
}

template<> template<typename U> inline auto array_view<uint8_t>::readm(U& value, uint size) -> U {
  value = 0;
  for(uint byte : reverse(range(size))) value |= (U)read() << byte * 8;
  return value;
}

template<> template<typename U> inline auto array_view<uint8_t>::readvn(U& value, uint size) -> U {
  value = 0;
  uint shift = 1;
  while(true) {
    auto byte = read();
    value += (byte & 0x7f) * shift;
    if(byte & 0x80) break;
    shift <<= 7;
    value += shift;
  }
  return value;
}

template<> template<typename U> inline auto array_view<uint8_t>::readvi(U& value, uint size) -> U {
  value = readvn<U>();
  bool negate = value & 1;
  value >>= 1;
  if(negate) value = ~value;
  return value;
}

}
