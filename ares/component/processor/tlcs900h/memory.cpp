#define PC r.pc.l.l0

template<> auto TLCS900H::fetch< uint8>() ->  uint8 {
//while(p.valid < 3) prefetch();
  if(p.valid == 0) prefetch();
  uint8 data = p.data;
  p.data >>= 8;
  p.valid--;
  return PC++, data;
}

template<> auto TLCS900H::fetch<uint16>() -> uint16 {
  uint16 data = fetch<uint8>() << 0;
  return data | fetch<uint8>() << 8;
}

template<> auto TLCS900H::fetch<uint24>() -> uint24 {
  uint24 data  = fetch<uint8>() <<  0;
         data |= fetch<uint8>() <<  8;
  return data |= fetch<uint8>() << 16;
}

template<> auto TLCS900H::fetch<uint32>() -> uint32 {
  uint32 data  = fetch<uint8>() <<  0;
         data |= fetch<uint8>() <<  8;
         data |= fetch<uint8>() << 16;
  return data |= fetch<uint8>() << 24;
}

#undef PC

template<> auto TLCS900H::fetch< int8>() ->  int8 { return ( int8)fetch< uint8>(); }
template<> auto TLCS900H::fetch<int16>() -> int16 { return (int16)fetch<uint16>(); }
template<> auto TLCS900H::fetch<int24>() -> int24 { return (int24)fetch<uint24>(); }
template<> auto TLCS900H::fetch<int32>() -> int32 { return (int32)fetch<uint32>(); }

template<typename T> auto TLCS900H::fetchRegister() -> Register<T> { return Register<T>{fetch<uint8>()}; }
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

template<> auto TLCS900H::load(Memory< uint8> memory) ->  uint8 { return read(Byte, memory.address); }
template<> auto TLCS900H::load(Memory<uint16> memory) -> uint16 { return read(Word, memory.address); }
template<> auto TLCS900H::load(Memory<uint32> memory) -> uint32 { return read(Long, memory.address); }

template<> auto TLCS900H::load(Memory< int8> memory) ->  int8 { return  (int8)read(Byte, memory.address); }
template<> auto TLCS900H::load(Memory<int16> memory) -> int16 { return (int16)read(Word, memory.address); }
template<> auto TLCS900H::load(Memory<int32> memory) -> int32 { return (int32)read(Long, memory.address); }

template<> auto TLCS900H::store(Memory< uint8> memory, uint32 data) -> void { write(Byte, memory.address, data); }
template<> auto TLCS900H::store(Memory<uint16> memory, uint32 data) -> void { write(Word, memory.address, data); }
template<> auto TLCS900H::store(Memory<uint32> memory, uint32 data) -> void { write(Long, memory.address, data); }
