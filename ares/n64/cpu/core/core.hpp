//{
  //core.cpp
  auto powerR4300() -> void;

  //memory.cpp
  auto readByte   (u32 address) -> maybe<u32>;
  auto readHalf   (u32 address) -> maybe<u32>;
  auto readWord   (u32 address) -> maybe<u32>;
  auto readDouble (u32 address) -> maybe<u64>;
  auto writeByte  (u32 address,  u8 data) -> void;
  auto writeHalf  (u32 address, u16 data) -> void;
  auto writeWord  (u32 address, u32 data) -> void;
  auto writeDouble(u32 address, u64 data) -> void;

  union r64 {
    struct {   int32_t order_msb2(i32h, i32); };
    struct {  uint32_t order_msb2(u32h, u32); };
    struct { float32_t order_msb2(f32h, f32); };
    struct {   int64_t i64; };
    struct {  uint64_t u64; };
    struct { float64_t f64; };
    auto i128() const ->  int128_t { return  (int128_t)i64; }
    auto u128() const -> uint128_t { return (uint128_t)u64; }
  };

  enum Interrupt : uint {
    Software0 = 0,
    Software1 = 1,
    RCP       = 2,
    Cartridge = 3,
    Reset     = 4,
    ReadRDB   = 5,
    WriteRDB  = 6,
    Timer     = 7,
  };

  //serialization.cpp
  auto serializeR4300(serializer&) -> void;

  #include "instruction.hpp"
  #include "exception.hpp"
  #include "pipeline.hpp"
  #include "cpu-registers.hpp"
  #include "scc-registers.hpp"
  #include "fpu-registers.hpp"
  #include "disassembler.hpp"

  static constexpr bool Endian = 1;  //0 = little, 1 = big
  static constexpr uint FlipLE = (Endian == 0 ? 7 : 0);
  static constexpr uint FlipBE = (Endian == 1 ? 7 : 0);
//};
