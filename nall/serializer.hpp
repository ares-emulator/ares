#pragma once

//serializer: a class designed to save and restore the state of classes.
//
//benefits:
//- data() will be portable in size (it is not necessary to specify type sizes.)
//- data() will be portable in endianness (always stored internally as little-endian.)
//- one serialize function can both save and restore class states.
//
//caveats:
//- only plain-old-data can be stored. complex classes must provide serialize(serializer&);
//- floating-point usage is not portable across different implementations

#include <nall/array.hpp>
#include <nall/bit.hpp>
#include <nall/range.hpp>
#include <nall/stdint.hpp>
#include <nall/traits.hpp>
#include <nall/utility.hpp>

namespace nall {

struct serializer;

template<typename T>
struct has_serialize {
  template<typename C> static auto test(decltype(std::declval<C>().serialize(std::declval<serializer&>()))*) -> char;
  template<typename C> static auto test(...) -> long;
  static constexpr bool value = sizeof(test<T>(0)) == sizeof(char);
};
template<typename T> constexpr bool has_serialize_v = has_serialize<T>::value;

struct serializer {
  explicit operator bool() const {
    return _size;
  }

  auto reading() const -> bool {
    return _mode == 0;
  }

  auto writing() const -> bool {
    return _mode == 1;
  }

  auto setReading() -> void {
    _mode = 0;
    _size = 0;
  }

  auto setWriting() -> void {
    _mode = 1;
    _size = 0;
  }

  auto data() const -> const uint8_t* {
    return _data;
  }

  auto size() const -> uint {
    return _size;
  }

  auto capacity() const -> uint {
    return _capacity;
  }

  auto reserve(uint size) -> void {
    if(size > _capacity) {
      auto data = new uint8_t[bit::round(size)]();
      memory::copy(data, _data, _capacity);
      delete[] _data;
      _data = data;
      _capacity = bit::round(size);
    }
  }

  template<typename T> auto operator()(T& value) -> serializer& {
    static_assert(has_serialize_v<T> || is_integral_v<T> || is_floating_point_v<T>);
    if constexpr(has_serialize_v<T>) {
      value.serialize(*this);
    } else if constexpr(is_integral_v<T>) {
      integer(value);
    } else if constexpr(is_floating_point_v<T>) {
      real(value);
    }
    return *this;
  }

  template<typename T, int N> auto operator()(T (&array)[N]) -> serializer& {
    for(auto& value : array) operator()(value);
    return *this;
  }

  template<typename T> auto operator()(array_span<T> array) -> serializer& {
    for(auto& value : array) operator()(value);
    return *this;
  }

  auto operator=(const serializer& s) -> serializer& {
    if(_data) delete[] _data;

    _mode = s._mode;
    _data = new uint8_t[s._capacity];
    _size = s._size;
    _capacity = s._capacity;

    memory::copy(_data, s._data, s._capacity);
    return *this;
  }

  auto operator=(serializer&& s) -> serializer& {
    if(_data) delete[] _data;

    _mode = s._mode;
    _data = s._data;
    _size = s._size;
    _capacity = s._capacity;

    s._data = nullptr;
    return *this;
  }

  serializer(const serializer& s) { operator=(s); }
  serializer(serializer&& s) { operator=(move(s)); }

  serializer() {
    setWriting();
    _data = new uint8_t[1024 * 1024]();
    _size = 0;
    _capacity = 1024 * 1024;
  }

  serializer(const uint8_t* data, uint capacity) {
    setReading();
    _data = new uint8_t[capacity]();
    _size = 0;
    _capacity = capacity;
    memory::copy(_data, data, capacity);
  }

  ~serializer() {
    if(_data) delete[] _data;
  }

private:
  template<typename T> auto integer(T& value) -> serializer& {
    enum : uint { size = std::is_same<bool, T>::value ? 1 : sizeof(T) };
    reserve(_size + size);
    if(writing()) {
      T copy = value;
      for(uint n : range(size)) _data[_size++] = copy, copy >>= 8;
    } else if(reading()) {
      value = 0;
      for(uint n : range(size)) value |= (T)_data[_size++] << (n << 3);
    }
    return *this;
  }

  template<typename T> auto real(T& value) -> serializer& {
    enum : uint { size = sizeof(T) };
    reserve(_size + size);
    //this is rather dangerous, and not cross-platform safe;
    //but there is no standardized way to export floating point values
    auto p = (uint8_t*)&value;
    if(writing()) {
      for(uint n : range(size)) _data[_size++] = p[n];
    } else if(reading()) {
      for(uint n : range(size)) p[n] = _data[_size++];
    }
    return *this;
  }

  bool _mode = 0;
  uint8_t* _data = nullptr;
  uint _size = 0;
  uint _capacity = 0;
};

}
