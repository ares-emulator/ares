#pragma once

namespace nall {

template<uint Precision> struct Integer {
  static_assert(Precision >= 1 && Precision <= 64);
  static constexpr auto bits() -> uint { return Precision; }
  using stype =
    conditional_t<Precision <=  8,  int8_t,
    conditional_t<Precision <= 16, int16_t,
    conditional_t<Precision <= 32, int32_t,
    conditional_t<Precision <= 64, int64_t,
    void>>>>;
  using utype = typename Natural<Precision>::utype;
  static constexpr auto mask() -> utype { return ~0ull >> 64 - Precision; }
  static constexpr auto sign() -> utype { return 1ull << Precision - 1; }

  Integer() : data(0) {}
  template<uint Bits> Integer(Integer<Bits> value) { data = cast(value); }
  template<typename T> Integer(const T& value) { data = cast(value); }
  explicit Integer(const char* value) { data = cast(toInteger(value)); }

  operator stype() const { return data; }

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

  auto bit(int index) -> DynamicBitRange<Integer> { return {*this, index}; }
  auto bit(int index) const -> const DynamicBitRange<Integer> { return {(Integer&)*this, index}; }

  auto bit(int lo, int hi) -> DynamicBitRange<Integer> { return {*this, lo, hi}; }
  auto bit(int lo, int hi) const -> const DynamicBitRange<Integer> { return {(Integer&)*this, lo, hi}; }

  auto byte(int index) -> DynamicBitRange<Integer> { return {*this, index * 8 + 0, index * 8 + 7}; }
  auto byte(int index) const -> const DynamicBitRange<Integer> { return {(Integer&)*this, index * 8 + 0, index * 8 + 7}; }

  auto mask(int index) const -> utype {
    return data & 1 << index;
  }

  auto mask(int lo, int hi) const -> utype {
    return data & (~0ull >> 64 - (hi - lo + 1) << lo);
  }

  auto slice(int index) const { return Natural<>{bit(index)}; }
  auto slice(int lo, int hi) const { return Natural<>{bit(lo, hi)}; }

  auto clamp(uint bits) -> stype {
    const int64_t b = 1ull << bits - 1;
    const int64_t m = b - 1;
    return data > m ? m : data < -b ? -b : data;
  }

  auto clip(uint bits) -> stype {
    const uint64_t b = 1ull << bits - 1;
    const uint64_t m = b * 2 - 1;
    return (data & m ^ b) - b;
  }

  auto serialize(serializer& s) { s(data); }
  auto natural() const -> Natural<Precision>;

private:
  auto cast(stype value) const -> stype {
    return (value & mask() ^ sign()) - sign();
  }

  stype data;
};

}
