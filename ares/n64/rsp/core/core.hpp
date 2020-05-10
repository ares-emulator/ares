//{
  //core.cpp
  auto powerCore() -> void;

  //memory.cpp
  auto readByte(u32 address) -> maybe<u32>;
  auto readHalf(u32 address) -> maybe<u32>;
  auto readWord(u32 address) -> maybe<u32>;
  auto writeByte(u32 address,  u8 data) -> void;
  auto writeHalf(u32 address, u16 data) -> void;
  auto writeWord(u32 address, u32 data) -> void;

  union r64 {
    struct {  int32_t order_msb2(i32h, i32); };
    struct { uint32_t order_msb2(u32h, u32); };
    struct {  int64_t i64; };
    struct { uint64_t u64; };
  };

  union v128 {
    struct { uint128_t u128; };
    struct {   __m128i s128; };
    operator __m128i() const { return s128; }
    auto operator=(__m128i value) { s128 = value; }
    auto byte   (uint index) ->  u8& { return  ((u8*)&u128)[15 - index]; }
    auto element(uint index) -> u16& { return ((u16*)&u128)[ 7 - index]; }
  };

  //serialization.cpp
  auto serializeCore(serializer&) -> void;

  #include "instruction.hpp"
  #include "pipeline.hpp"
  #include "cpu-registers.hpp"
  #include "scc-registers.hpp"
  #include "vu-registers.hpp"
  #include "disassembler.hpp"
//};
