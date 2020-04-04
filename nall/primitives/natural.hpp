#pragma once

namespace nall {

template<uint Precision> struct Natural {
  static_assert(Precision >= 1 && Precision <= 64);
  static constexpr auto bits() -> uint { return Precision; }
  using utype =
    conditional_t<Precision <=  8,  uint8_t,
    conditional_t<Precision <= 16, uint16_t,
    conditional_t<Precision <= 32, uint32_t,
    conditional_t<Precision <= 64, uint64_t,
    void>>>>;
  static constexpr auto mask() -> utype { return ~0ull >> 64 - Precision; }

  Natural() : data(0) {}
  template<uint Bits> Natural(Natural<Bits> value) { data = cast(value); }
  template<typename T> Natural(const T& value) { data = cast(value); }
  explicit Natural(const char* value) { data = cast(toNatural(value)); }

  operator utype() const { return data; }

  auto operator++(int) { auto value = *this; data = cast(data + 1); return value; }
  auto operator--(int) { auto value = *this; data = cast(data - 1); return value; }

  auto& operator++() { data = cast(data + 1); return *this; }
  auto& operator--() { data = cast(data - 1); return *this; }

  template<typename T> auto& operator  =(const T& value) { data = cast(        value); return *this; }
  template<typename T> auto& operator *=(const T& value) { data = cast(data  * value); return *this; }
  template<typename T> auto& operator /=(const T& value) { data = cast(data  / value); return *this; }
  template<typename T> auto& operator %=(const T& value) { data = cast(data  % value); return *this; }
  template<typename T> auto& operator +=(const T& value) { data = cast(data  + value); return *this; }
  template<typename T> auto& operator -=(const T& value) { data = cast(data  - value); return *this; }
  template<typename T> auto& operator<<=(const T& value) { data = cast(data << value); return *this; }
  template<typename T> auto& operator>>=(const T& value) { data = cast(data >> value); return *this; }
  template<typename T> auto& operator &=(const T& value) { data = cast(data  & value); return *this; }
  template<typename T> auto& operator ^=(const T& value) { data = cast(data  ^ value); return *this; }
  template<typename T> auto& operator |=(const T& value) { data = cast(data  | value); return *this; }

  auto bit(int index) -> DynamicBitRange<Natural> { return {*this, index}; }
  auto bit(int index) const -> const DynamicBitRange<Natural> { return {(Natural&)*this, index}; }

  auto bit(int lo, int hi) -> DynamicBitRange<Natural> { return {*this, lo, hi}; }
  auto bit(int lo, int hi) const -> const DynamicBitRange<Natural> { return {(Natural&)*this, lo, hi}; }

  auto byte(int index) -> DynamicBitRange<Natural> { return {*this, index * 8 + 0, index * 8 + 7}; }
  auto byte(int index) const -> const DynamicBitRange<Natural> { return {(Natural&)*this, index * 8 + 0, index * 8 + 7}; }

  auto mask(int index) const -> utype {
    return data & 1 << index;
  }

  auto mask(int lo, int hi) const -> utype {
    return data & (~0ull >> 64 - (hi - lo + 1) << lo);
  }

  auto slice(int index) const { return Natural<>{bit(index)}; }
  auto slice(int lo, int hi) const { return Natural<>{bit(lo, hi)}; }

  auto clamp(uint bits) -> utype {
    const uint64_t b = 1ull << bits - 1;
    const uint64_t m = b * 2 - 1;
    return data < m ? data : m;
  }

  auto clip(uint bits) -> utype {
    const uint64_t b = 1ull << bits - 1;
    const uint64_t m = b * 2 - 1;
    return data & m;
  }

  auto serialize(serializer& s) { s(data); }
  auto integer() const -> Integer<Precision>;

private:
  auto cast(utype value) const -> utype {
    return value & mask();
  }

  utype data;
};

}
