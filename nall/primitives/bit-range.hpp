#pragma once

namespace nall {

/* static BitRange */

template<int Precision, int Lo, int Hi> struct BitRange {
  static_assert(Precision >= 1 && Precision <= 64);
  static_assert(Lo < Precision && Hi < Precision);
  static_assert(Lo <= Hi);
  using type =
    conditional_t<Precision <=  8,  uint8_t,
    conditional_t<Precision <= 16, uint16_t,
    conditional_t<Precision <= 32, uint32_t,
    conditional_t<Precision <= 64, uint64_t,
    void>>>>;
  enum : uint { lo = Lo < 0 ? Precision + Lo : Lo };
  enum : uint { hi = Hi < 0 ? Precision + Hi : Hi };
  enum : type { mask = ~0ull >> 64 - (hi - lo + 1) << lo };
  enum : uint { shift = lo };

  BitRange(const BitRange& source) = delete;

  auto& operator=(const BitRange& source) {
    target = target & ~mask | ((source.target & source.mask) >> source.shift) << shift & mask;
    return *this;
  }

  template<typename T> BitRange(T* source) : target((type&)*source) {
    static_assert(sizeof(T) == sizeof(type));
  }

  operator type() const {
    return (target & mask) >> shift;
  }

  auto operator++(int) {
    auto value = (target & mask) >> shift;
    target = target & ~mask | target + (1 << shift) & mask;
    return value;
  }

  auto operator--(int) {
    auto value = (target & mask) >> shift;
    target = target & ~mask | target - (1 << shift) & mask;
    return value;
  }

  auto& operator++() {
    target = target & ~mask | target + (1 << shift) & mask;
    return *this;
  }

  auto& operator--() {
    target = target & ~mask | target - (1 << shift) & mask;
    return *this;
  }

  template<typename T> auto& operator=(const T& source) {
    target = target & ~mask | source << shift & mask;
    return *this;
  }

  template<typename T> auto& operator*=(const T& source) {
    auto value = ((target & mask) >> shift) * source;
    target = target & ~mask | value << shift & mask;
    return *this;
  }

  template<typename T> auto& operator/=(const T& source) {
    auto value = ((target & mask) >> shift) / source;
    target = target & ~mask | value << shift & mask;
    return *this;
  }

  template<typename T> auto& operator%=(const T& source) {
    auto value = ((target & mask) >> shift) % source;
    target = target & ~mask | value << shift & mask;
    return *this;
  }

  template<typename T> auto& operator+=(const T& source) {
    auto value = ((target & mask) >> shift) + source;
    target = target & ~mask | value << shift & mask;
    return *this;
  }

  template<typename T> auto& operator-=(const T& source) {
    auto value = ((target & mask) >> shift) - source;
    target = target & ~mask | value << shift & mask;
    return *this;
  }

  template<typename T> auto& operator<<=(const T& source) {
    auto value = ((target & mask) >> shift) << source;
    target = target & ~mask | value << shift & mask;
    return *this;
  }

  template<typename T> auto& operator>>=(const T& source) {
    auto value = ((target & mask) >> shift) >> source;
    target = target & ~mask | value << shift & mask;
    return *this;
  }

  template<typename T> auto& operator&=(const T& source) {
    target = target & (~mask | source << shift & mask);
    return *this;
  }

  template<typename T> auto& operator^=(const T& source) {
    target = target ^ source << shift & mask;
    return *this;
  }

  template<typename T> auto& operator|=(const T& source) {
    target = target | source << shift & mask;
    return *this;
  }

private:
  type& target;
};

/* dynamic BitRange */

template<typename Type, int Precision = Type::bits()> struct DynamicBitRange {
  static_assert(Precision >= 1 && Precision <= 64);
  using type =
    conditional_t<Precision <=  8,  uint8_t,
    conditional_t<Precision <= 16, uint16_t,
    conditional_t<Precision <= 32, uint32_t,
    conditional_t<Precision <= 64, uint64_t,
    void>>>>;

  DynamicBitRange(const DynamicBitRange& source) = delete;

  auto& operator=(const DynamicBitRange& source) {
    target = target & ~mask | ((source.target & source.mask) >> source.shift) << shift & mask;
    return *this;
  }

  DynamicBitRange(Type& source, int index) : target(source) {
    if(index < 0) index = Precision + index;
    mask = 1ull << index;
    shift = index;
  }

  DynamicBitRange(Type& source, int lo, int hi) : target(source) {
    if(lo < 0) lo = Precision + lo;
    if(hi < 0) hi = Precision + hi;
    if(lo > hi) swap(lo, hi);
    mask = ~0ull >> 64 - (hi - lo + 1) << lo;
    shift = lo;
  }

  operator type() const {
    return (target & mask) >> shift;
  }

  auto operator++(int) {
    auto value = (target & mask) >> shift;
    target = target & ~mask | target + (1 << shift) & mask;
    return value;
  }

  auto operator--(int) {
    auto value = (target & mask) >> shift;
    target = target & ~mask | target - (1 << shift) & mask;
    return value;
  }

  auto& operator++() {
    target = target & ~mask | target + (1 << shift) & mask;
    return *this;
  }

  auto& operator--() {
    target = target & ~mask | target - (1 << shift) & mask;
    return *this;
  }

  template<typename T> auto& operator=(const T& source) {
    target = target & ~mask | source << shift & mask;
    return *this;
  }

  template<typename T> auto& operator*=(const T& source) {
    auto value = ((target & mask) >> shift) * source;
    target = target & ~mask | value << shift & mask;
    return *this;
  }

  template<typename T> auto& operator/=(const T& source) {
    auto value = ((target & mask) >> shift) / source;
    target = target & ~mask | value << shift & mask;
    return *this;
  }

  template<typename T> auto& operator%=(const T& source) {
    auto value = ((target & mask) >> shift) % source;
    target = target & ~mask | value << shift & mask;
    return *this;
  }

  template<typename T> auto& operator+=(const T& source) {
    auto value = ((target & mask) >> shift) + source;
    target = target & ~mask | value << shift & mask;
    return *this;
  }

  template<typename T> auto& operator-=(const T& source) {
    auto value = ((target & mask) >> shift) - source;
    target = target & ~mask | value << shift & mask;
    return *this;
  }

  template<typename T> auto& operator<<=(const T& source) {
    auto value = ((target & mask) >> shift) << source;
    target = target & ~mask | value << shift & mask;
    return *this;
  }

  template<typename T> auto& operator>>=(const T& source) {
    auto value = ((target & mask) >> shift) >> source;
    target = target & ~mask | value << shift & mask;
    return *this;
  }

  template<typename T> auto& operator&=(const T& source) {
    target = target & (~mask | source << shift & mask);
    return *this;
  }

  template<typename T> auto& operator^=(const T& source) {
    target = target ^ source << shift & mask;
    return *this;
  }

  template<typename T> auto& operator|=(const T& source) {
    target = target | source << shift & mask;
    return *this;
  }

private:
  Type& target;
  type mask;
  uint shift;
};

}
