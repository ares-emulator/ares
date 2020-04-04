#pragma once

namespace nall {

template<uint Precision> struct Real {
  static_assert(Precision == 32 || Precision == 64);
  static constexpr auto bits() -> uint { return Precision; }
  using ftype =
    conditional_t<bits() == 32, float32_t,
    conditional_t<bits() == 64, float64_t,
    void>>;

  Real() : data(0.0) {}
  template<int Bits> Real(Real<Bits> value) : data((ftype)value) {}
  template<typename T> Real(const T& value) : data((ftype)value) {}
  explicit Real(const char* value) : data((ftype)toReal(value)) {}

  operator ftype() const { return data; }

  auto operator++(int) { auto value = *this; ++data; return value; }
  auto operator--(int) { auto value = *this; --data; return value; }

  auto& operator++() { data++; return *this; }
  auto& operator--() { data--; return *this; }

  template<typename T> auto& operator =(const T& value) { data =        value; return *this; }
  template<typename T> auto& operator*=(const T& value) { data = data * value; return *this; }
  template<typename T> auto& operator/=(const T& value) { data = data / value; return *this; }
  template<typename T> auto& operator%=(const T& value) { data = data % value; return *this; }
  template<typename T> auto& operator+=(const T& value) { data = data + value; return *this; }
  template<typename T> auto& operator-=(const T& value) { data = data - value; return *this; }

  auto serialize(serializer& s) { s(data); }

private:
  ftype data;
};

}
