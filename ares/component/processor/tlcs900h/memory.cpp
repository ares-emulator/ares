#define PC r.pc.l.l0

template<> auto TLCS900H::fetch<n8 >() -> n8  {
//while(p.valid < 3) prefetch();
  if(p.valid == 0) prefetch();
  n8 data = p.data;
  p.data >>= 8;
  p.valid--;
  return PC++, data;
}

template<> auto TLCS900H::fetch<n16>() -> n16 {
  n16    data = fetch<n8>() << 0;
  return data | fetch<n8>() << 8;
}

template<> auto TLCS900H::fetch<n24>() -> n24 {
  n24    data  = fetch<n8>() <<  0;
         data |= fetch<n8>() <<  8;
  return data |= fetch<n8>() << 16;
}

template<> auto TLCS900H::fetch<n32>() -> n32 {
  n32    data  = fetch<n8>() <<  0;
         data |= fetch<n8>() <<  8;
         data |= fetch<n8>() << 16;
  return data |= fetch<n8>() << 24;
}

#undef PC

template<> auto TLCS900H::fetch<i8 >() -> i8  { return (i8 )fetch<n8 >(); }
template<> auto TLCS900H::fetch<i16>() -> i16 { return (i16)fetch<n16>(); }
template<> auto TLCS900H::fetch<i24>() -> i24 { return (i24)fetch<n24>(); }
template<> auto TLCS900H::fetch<i32>() -> i32 { return (i32)fetch<n32>(); }

template<typename T> auto TLCS900H::fetchRegister() -> Register<T> { return Register<T>{fetch<n8>()}; }
template<typename T, typename U> auto TLCS900H::fetchMemory() -> Memory<T> { return Memory<T>{fetch<U>()}; }
template<typename T> auto TLCS900H::fetchImmediate() -> Immediate<T> { return Immediate<T>{fetch<T>()}; }

//

#define XSP r.xsp.l.l0

template<typename T> auto TLCS900H::pop(T data) -> void {
  auto value = typename T::type();
  if constexpr(T::bits ==  8) value = read(Byte, XSP);
  if constexpr(T::bits == 16) value = read(Word, XSP);
  if constexpr(T::bits == 32) value = read(Long, XSP);
  store(data, value);
  XSP += T::bits >> 3;
}

template<typename T> auto TLCS900H::push(T data) -> void {
  XSP -= T::bits >> 3;
  auto value = load(data);
  if constexpr(T::bits ==  8) write(Byte, XSP, value);
  if constexpr(T::bits == 16) write(Word, XSP, value);
  if constexpr(T::bits == 32) write(Long, XSP, value);
}

#undef XSP

//

template<> auto TLCS900H::load(Memory<n8 > memory) -> n8  { return read(Byte, memory.address); }
template<> auto TLCS900H::load(Memory<n16> memory) -> n16 { return read(Word, memory.address); }
template<> auto TLCS900H::load(Memory<n32> memory) -> n32 { return read(Long, memory.address); }

template<> auto TLCS900H::load(Memory<i8 > memory) -> i8  { return (i8 )read(Byte, memory.address); }
template<> auto TLCS900H::load(Memory<i16> memory) -> i16 { return (i16)read(Word, memory.address); }
template<> auto TLCS900H::load(Memory<i32> memory) -> i32 { return (i32)read(Long, memory.address); }

template<> auto TLCS900H::store(Memory<n8 > memory, n32 data) -> void { write(Byte, memory.address, data); }
template<> auto TLCS900H::store(Memory<n16> memory, n32 data) -> void { write(Word, memory.address, data); }
template<> auto TLCS900H::store(Memory<n32> memory, n32 data) -> void { write(Long, memory.address, data); }
