#pragma once

#include <nall/iterator.hpp>
#include <nall/range.hpp>
#include <nall/traits.hpp>
#include <vector>

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

  array_view(const void* data, u64 size) {
    _data = (const T*)data;
    _size = (s32)size;
  }

  template<s32 size> array_view(const T (&data)[size]) {
    _data = data;
    _size = size;
  }

  array_view(const std::vector<T>& v) {
    _data = v.data();
    _size = (s32)v.size();
  }

  auto operator-=(s32 distance) -> type& { _data -= distance; _size += distance; return *this; }
  auto operator+=(s32 distance) -> type& { _data += distance; _size -= distance; return *this; }

  auto operator[](u32 index) const -> const T& {
    #ifdef DEBUG
    struct out_of_bounds {};
    if(index >= _size) throw out_of_bounds{};
    #endif
    return _data[index];
  }

  

  template<typename U = T> auto data() const -> const U* { return (const U*)_data; }
  template<typename U = T> auto size() const -> u64 { return _size * sizeof(T) / sizeof(U); }

  auto begin() const -> iterator_const<T> { return {_data, (u32)0}; }
  auto end() const -> iterator_const<T> { return {_data, (u32)_size}; }


  auto read() -> T {
    auto value = operator[](0);
    _data++;
    _size--;
    return value;
  }

  

  //array_view<u8> specializations
  template<typename U> auto readm(U& value, u32 size) -> U;
  
  template<typename U = u64> auto readm(u32 size) -> U { U value; return readm(value, size); }
  

protected:
  const T* _data;
  s32 _size;
};

//array_view<u8>

template<> template<typename U> inline auto array_view<u8>::readm(U& value, u32 size) -> U {
  value = 0;
  for(u32 byte : reverse(range(size))) value |= (U)read() << byte * 8;
  return value;
}


}
