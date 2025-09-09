#pragma once

#include <nall/array-view.hpp>
#include <vector>

namespace nall {

template<typename T> struct array_span : array_view<T> {
  using type = array_span;
  using super = array_view<T>;

  array_span() {
    super::_data = nullptr;
    super::_size = 0;
  }

  array_span(nullptr_t) {
    super::_data = nullptr;
    super::_size = 0;
  }

  array_span(void* data, u64 size) {
    super::_data = (T*)data;
    super::_size = (s32)size;
  }

  template<s32 size> array_span(T (&data)[size]) {
    super::_data = data;
    super::_size = size;
  }

  array_span(std::vector<T>& v) {
    super::_data = v.data();
    super::_size = (s32)v.size();
  }

  auto operator[](u32 index) -> T& { return (T&)super::operator[](index); }

  template<typename U = T> auto data() -> U* { return (U*)super::_data; }
  template<typename U = T> auto data() const -> const U* { return (const U*)super::_data; }

  auto begin() -> iterator<T> { return {(T*)super::_data, (u32)0}; }
  auto end() -> iterator<T> { return {(T*)super::_data, (u32)super::_size}; }

  auto write(T value) -> void {
    operator[](0) = value;
    super::_data++;
    super::_size--;
  }

  auto span(u32 offset, u32 length) -> type {
    #ifdef DEBUG
    struct out_of_bounds {};
    if(offset + length >= super::_size) throw out_of_bounds{};
    #endif
    return {(T*)super::_data + offset, length};
  }

  //array_span<u8> specializations
  template<typename U> auto writel(U value, u32 size) -> void;
  template<typename U> auto writem(U value, u32 size) -> void;
};


}
