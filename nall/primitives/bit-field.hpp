#pragma once

namespace nall {

template<int...> struct BitField;

/* static BitField */

template<int Precision, int Index> struct BitField<Precision, Index> {
  static_assert(Precision >= 1 && Precision <= 64);
  using type =
    conditional_t<Precision <=  8,  uint8_t,
    conditional_t<Precision <= 16, uint16_t,
    conditional_t<Precision <= 32, uint32_t,
    conditional_t<Precision <= 64, uint64_t,
    void>>>>;
  enum : uint { shift = Index < 0 ? Precision + Index : Index };
  enum : type { mask = 1ull << shift };

  BitField(const BitField&) = delete;

  auto& operator=(const BitField& source) {
    target = target & ~mask | (bool)source << shift;
    return *this;
  }

  template<typename T> BitField(T* source) : target((type&)*source) {
    static_assert(sizeof(T) == sizeof(type));
  }

  auto bit() const {
    return shift;
  }

  operator bool() const {
    return target & mask;
  }

  auto& operator=(bool source) {
    target = target & ~mask | source << shift;
    return *this;
  }

  auto& operator&=(bool source) {
    target = target & (~mask | source << shift);
    return *this;
  }

  auto& operator^=(bool source) {
    target = target ^ source << shift;
    return *this;
  }

  auto& operator|=(bool source) {
    target = target | source << shift;
    return *this;
  }

private:
  type& target;
};

/* dynamic BitField */

template<int Precision> struct BitField<Precision> {
  static_assert(Precision >= 1 && Precision <= 64);
  using type =
    conditional_t<Precision <=  8,  uint8_t,
    conditional_t<Precision <= 16, uint16_t,
    conditional_t<Precision <= 32, uint32_t,
    conditional_t<Precision <= 64, uint64_t,
    void>>>>;

  BitField(const BitField&) = delete;

  auto& operator=(const BitField& source) {
    target = target & ~mask | (bool)source << shift;
    return *this;
  }

  template<typename T> BitField(T* source, int index) : target((type&)*source) {
    static_assert(sizeof(T) == sizeof(type));
    if(index < 0) index = Precision + index;
    mask = 1ull << index;
    shift = index;
  }

  auto bit() const {
    return shift;
  }

  operator bool() const {
    return target & mask;
  }

  auto& operator=(bool source) {
    target = target & ~mask | source << shift;
    return *this;
  }

  auto& operator&=(bool source) {
    target = target & (~mask | source << shift);
    return *this;
  }

  auto& operator^=(bool source) {
    target = target ^ source << shift;
    return *this;
  }

  auto& operator|=(bool source) {
    target = target | source << shift;
    return *this;
  }

private:
  type& target;
  type mask;
  uint shift;
};

}
